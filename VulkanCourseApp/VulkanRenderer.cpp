#include "VulkanRenderer.h"



VulkanRenderer::VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow * newWindow)
{
	window = newWindow;

	try {
		createInstance();
		getPhysicalDevice();
		createLogicalDevice();
	}
	catch (const std::runtime_error &e) {
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return 0;
}

void VulkanRenderer::cleanup()
{
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}


VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::createInstance()
{
	// Information about the application itself
	// Most data here doesn't affect the program and is for developer convenience
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";						// Custom name of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);			// Custom version of the application
	appInfo.pEngineName = "No engine";								// Custom engine name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);				// Custom engine version
	appInfo.apiVersion = VK_API_VERSION_1_0;						// Vulkan version

	//Creation information for a VkInstance (Vulkan Instance)
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	//Create list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	//set up extensions Instance will use
	uint32_t glfwExtensionCount = 0; //GLFW may require multiple extensions
	const char** glfwExtensions;     //Extensions passed as array of cstrings, so need pointer (the array) to pointer (the cstring)

	//Get glfw extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	//add GLFW extensions to list of extensions
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	//Check Instance extensions supported
	if (!checkInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	//TODO: Set up Validation layers that Instance will use
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	//Create Instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance!");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	// Get the queue family indices for the chosen Physical device
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// Queue the logical device needs to create and info to do so
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;			// The index of the family to create a queue from
	queueCreateInfo.queueCount = 1;										// number of queues to create
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;						// Vulkan needs to know how to handle multiple queues with different priorities

	// information to create logical device (sometimes called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;							// number of queue create infos
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;				// list of queue create infos so device can create required queues
	deviceCreateInfo.enabledExtensionCount = 0;							// number of enabled logicall device extensions
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;					// list of enabled logical device extensions
	
	// Physical device features the logical device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;				// Physical device features logical device will be using

	// Create the logical device given the physical device
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Logical Device!");
	}

	// Queues are created at the same time as the device...
	// So we want handle to queues.
	// From given logical device, of given queue family, of given queue index (0 since there is only one here), place reference in given vkqueue
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
}

void VulkanRenderer::getPhysicalDevice()
{	
	//Enumarate Physical device the vkInstance can access
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	//if no devices are available then none support Vulkan
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPUs that support Vulkan Instance");
	}

	// Get List of physical devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	for (const auto &device : deviceList)
	{
		if (checkDeviceSuitable(device))
		{
			mainDevice.physicalDevice = device;
			break;
		}
	}
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	// need to get number of extensions to create array of correct size to hold extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	
	// Create a list of vkExtensionProperties using count
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	//check if given extensions are in list of available extensions
	for (const auto &checkExtension : *checkExtensions)
	{
		bool hasExtension = false;
		for (const auto &extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName))
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	/*
	// Information bout the device itself (ID, name, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	//Information about what the device can do (geo shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	QueueFamilyIndices indices = getQueueFamilies(device);

	return indices.isValid();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	// Get all Queue family property info for the given device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	//Go through each queue family and check if it has at least 1 of the required types of queue
	int i = 0;
	for (const auto &queueFamily : queueFamilyList)
	{
		// first check if queue family has at least one queue in that family (could have no queues)
		// Queue can be multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT
		// to check if it has required type
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i; // If queue family is valid, then get index
		}

		// Check if queue family idices are in a valid state, stop searching if so
		if (indices.isValid())
		{
			break;
		}

		i++;
	}

	return indices;
}
