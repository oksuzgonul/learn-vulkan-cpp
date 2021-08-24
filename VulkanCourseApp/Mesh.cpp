#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices)
{
	vertexCount = vertices->size();
	indexCount = indices->size();
	physicalDevice = newPhysicalDevice;
	device = newDevice;
	createVertexBuffer(transferQueue, transferCommandPool, vertices);
	createIndexBuffer(transferQueue, transferCommandPool, indices);

	uboModel.model = glm::mat4(1.0f);
}

void Mesh::setModel(glm::mat4 newModel)
{
	uboModel.model = newModel;
}

UboModel Mesh::getModel()
{
	return uboModel;
}

int Mesh::getVertexCount()
{
	return vertexCount;
}

int Mesh::getIndexCount()
{
	return indexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertexBuffer;
}

VkBuffer Mesh::getIndexBuffer()
{
	return indexBuffer;
}

void Mesh::destroyBuffers()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
}

Mesh::~Mesh()
{

}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create Buffer and Allocate Memory to it
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory);



	//  -- Map memory to vertex buffer --
	void * data;																		// 1. create pointer to point in normal memory
	vkMapMemory(device, stagingBufferMemory, 0,bufferSize, 0, &data);					// 2. map the staging buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize);									// 3. copy memory from vertices vector to the point
	vkUnmapMemory(device, stagingBufferMemory);											// 4. Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not CPU (host)
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);
	
	// copy staging buffer to vertex buffer on GPU
	copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

	// clean up staging buffer parts
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	// Get size of buffer needed for indices
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// Temporary buffer to "stage" index data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	//  -- Map memory to vertex buffer --
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices->data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);
	
	// Create buffer for INDEX data on GPU access only area
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

	// copy staging buffer to index buffer on GPU
	copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

	// clean up staging buffer parts
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}
