#include "MeshModel.h"

MeshModel::MeshModel()
{
}

MeshModel::MeshModel(std::vector<Mesh> newMeshList)
{
	meshList = newMeshList;
	model = glm::mat4(1.0f);
}

size_t MeshModel::getMeshCount()
{
	return meshList.size();
}

Mesh* MeshModel::getMesh(size_t index)
{
	if (index >= meshList.size())
	{
		throw std::runtime_error("Attempted to access invalid Mesh index!");
	}

	return &meshList[index];
}

glm::mat4 MeshModel::getModel()
{
	return model;
}

void MeshModel::setModel(glm::mat4 newModel)
{
	model = newModel;
}

void MeshModel::destroyMeshModel()
{
	for (auto &mesh : meshList)
	{
		mesh.destroyBuffers();
	}
}

std::vector<std::string> MeshModel::LoadMaterials(const aiScene* scene)
{
	// create 1:1 sized list of textures
	std::vector<std::string> textureList(scene->mNumMaterials);

	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		// get the material
		aiMaterial* material = scene->mMaterials[i];

		// initialize the texture to empty string (will be replaced if texture exits)
		textureList[i] = "";

		// Check for a diffuse texture
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			// get the path of the texture file
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0 , &path) == AI_SUCCESS)
			{
				// cut off any directory information already present
				int idx = std::string(path.data).rfind("\\");
				std::string fileName = std::string(path.data).substr(idx + 1);
				textureList[i] = fileName;
			}
		}
	}

	return textureList;
}

std::vector<Mesh> MeshModel::LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, 
	VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex)
{
	std::vector<Mesh> meshList;

	// go through each mesh at this node and create it, then add it to our meshList
	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		meshList.push_back(LoadMesh(newPhysicalDevice, newDevice,
			transferQueue, transferCommandPool, scene->mMeshes[node->mMeshes[i]], scene, matToTex));
	}

	// go through each node attached to this load, then append their meshes to this node's mesh list
	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		std::vector<Mesh> newList = LoadNode(newPhysicalDevice, newDevice, 
			transferQueue, transferCommandPool, node->mChildren[i], scene, matToTex);
		meshList.insert(meshList.end(), newList.begin(), newList.end());
	}

	return meshList;
}

Mesh MeshModel::LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, 
	VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	// resize vertex list to hold all vertices for mesh
	vertices.resize(mesh->mNumVertices);

	// go through each vertex and copy it across to our vertices
	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		// set position
		vertices[i].pos = { mesh->mVertices->x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		// set tex coord if they exist
		if (mesh->mTextureCoords[0])
		{
			vertices[i].tex = { mesh->mTextureCoords[0][i].x,mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertices[i].tex = { 0.0f, 0.0f };
		}

		// set color (just use white for now)
		vertices[i].col = { 1.0f, 1.0f, 1.0f };
	}
	// iterate iver indices through faces and copy across
	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		// get a face
		aiFace face = mesh->mFaces[i];
		
		// go through face's indices and add to list 
		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// create new mesh with details and return in
	Mesh newMesh = Mesh(newPhysicalDevice, newDevice,
		transferQueue, transferCommandPool, &vertices, &indices, matToTex[mesh->mMaterialIndex]);

	return newMesh;
}

MeshModel::~MeshModel()
{
}
