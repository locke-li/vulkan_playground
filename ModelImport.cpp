#include "ModelImport.h"
#include "MeshNode.h"
#include "glm.hpp"
#include "gtc/type_ptr.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include <unordered_map>
#include <iostream>

bool LoadImageData(tinygltf::Image* image, const int image_idx, std::string* err,
	std::string* warn, int req_width, int req_height,
	const unsigned char* bytes, int size, void* userData) {
	std::cout << image_idx << " " << image->name << std::endl;
	//TODO stub
	return true;
}

bool stringEndsWith(const std::string& value, const std::string& suffix) {
	//TODO case insensitive compare
	if (value.length() >= suffix.length()) {
		return value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0;
	}
	return false;
}

///
/// obj format files are loaded as a single buffer, single bufferView mesh
///
bool ModelImport::loadObj(const char* path, const float scale, MeshInput* const meshOut) const {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapeList;
	std::vector<tinyobj::material_t> materialList;
	std::string warning, error;

	if (!tinyobj::LoadObj(&attrib, &shapeList, &materialList, &warning, &error, path)) {
		std::cout << warning << "\n----\n" << error << std::endl;
		return false;
	}
	if (!warning.empty()) {
		std::cout << warning << std::endl;
	}

	meshOut->reserve(shapeList.size());
	std::unordered_map<Vertex, uint32_t> vertexIndex;
	std::vector<std::vector<uint8_t>> buffer(1);
	std::vector<VertexIndexed> dataList;
	dataList.reserve(shapeList.size());
	for (const auto& shape : shapeList) {
		std::cout << shape.name << "\n";
		VertexIndexed data;
		for (const auto& indexInfo : shape.mesh.indices) {
			Vertex vertex{};
			auto index = indexInfo.vertex_index * 3;
			vertex.pos = {
				attrib.vertices[index] / scale,
				attrib.vertices[index + 1] / scale,
				attrib.vertices[index + 2] / scale,
			};
			index = indexInfo.texcoord_index * 2;
			vertex.texCoord = {
				attrib.texcoords[index],
				1.0f - attrib.texcoords[index + 1],
			};
			//TODO other attributes

			if (vertexIndex.count(vertex) == 0) {//unique vertex
				auto index = static_cast<uint32_t>(data.vertices.size());
				vertexIndex[vertex] = index;
				data.vertices.push_back(vertex);
				data.indices.push_back(index);
			}
			else {
				data.indices.push_back(vertexIndex[vertex]);
			}
		}
		std::cout << data.vertices.size() << "|" << data.indices.size() << std::endl;
		dataList.push_back(std::move(data));
	}
	meshOut->setMesh({ std::move(dataList) });
	//TODO manage material/texture
	return true;
}

bool loadGltfNode(const tinygltf::Model& model, const int nodeIndex, const float scale, std::vector<std::vector<VertexIndexed>>& meshDataList) {
	const auto& node = model.nodes[nodeIndex];
	//TODO apply scale here?
	//TODO set these values
	if (node.translation.size() == 3) {
		glm::vec3 translation = glm::make_vec3(node.translation.data());
	}
	if (node.rotation.size() == 4) {
		glm::quat rotation = glm::make_quat(node.rotation.data());
	}
	if (node.scale.size() == 3) {
		glm::vec3 scale = glm::make_vec3(node.scale.data());
	}
	if (node.matrix.size() == 16) {
		glm::mat4 trs = glm::make_mat4(node.matrix.data());
	}
	std::cout << nodeIndex << ":" << node.name << " ";
	if (node.mesh > -1) {
		std::cout << "mesh " << node.mesh << " ";
		const auto& mesh = model.meshes[node.mesh];
		std::vector<VertexIndexed> dataList;
		dataList.reserve(mesh.primitives.size());
		for (const auto& primitive : mesh.primitives) {
			//TODO support for mesh without indices
			if (primitive.indices < 0) continue;
			//glTF seems to pack the same attribute values in a continous bufferview
			//which is not a valid vertex array
			//thus we need to fill our Vertex array, this comes with the benefit of selectively choosing the vertex attributes to use
			const auto& posIter = primitive.attributes.find("POSITION");
			if (posIter == primitive.attributes.end()) {
				continue;
			}
			const float* texcoord0 = nullptr;
			const auto& accessorPos = model.accessors[posIter->second];
			const auto& bufferViewPos = model.bufferViews[accessorPos.bufferView];
			const auto* pos = reinterpret_cast<const float*>(&model.buffers[bufferViewPos.buffer].data[accessorPos.byteOffset + bufferViewPos.byteOffset]);
			const auto& texcoord0Iter = primitive.attributes.find("TEXCOORD_0");
			if (texcoord0Iter != primitive.attributes.end()) {
				const auto& accessorTexcoord0 = model.accessors[texcoord0Iter->second];
				const auto& bufferViewTexcoord0 = model.bufferViews[accessorTexcoord0.bufferView];
				texcoord0 = reinterpret_cast<const float*>(&model.buffers[bufferViewTexcoord0.buffer].data[accessorTexcoord0.byteOffset + bufferViewTexcoord0.byteOffset]);
			}
			const auto& accessorIndices = model.accessors[primitive.indices];
			const auto& bufferViewIndices = model.bufferViews[accessorIndices.bufferView];
			const void* indices = &model.buffers[bufferViewIndices.buffer].data[accessorIndices.byteOffset + bufferViewIndices.byteOffset];
			VertexIndexed data;
			data.vertices.reserve(accessorPos.count);
			for (auto i = 0; i < accessorPos.count; ++i) {
				Vertex vertex;
				vertex.pos = glm::make_vec3(reinterpret_cast<const float*>(pos + i * 3)) / scale;
				if (texcoord0) {
					vertex.texCoord = glm::make_vec2(texcoord0 + i * 2);
				}
				data.vertices.push_back(std::move(vertex));
			}

			switch (accessorIndices.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE: {
				const auto* indices8 = static_cast<const uint8_t*>(indices);
				for (auto i = 0; i < accessorIndices.count; ++i) {
					data.indices.push_back(indices8[i]);
				}
			}
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
				data.indices.resize(accessorIndices.count);
				memcpy(data.indices.data(), indices, accessorIndices.count * sizeof(uint16_t));
			}
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
				//TODO check if 32bit index is really needed?
				const auto* indices32 = static_cast<const uint32_t*>(indices);
				for (auto i = 0; i < accessorIndices.count; ++i) {
					data.indices.push_back(indices32[i]);
				}
			}
				break;
			default:
				std::cout << "unsupported index type: " << accessorIndices.componentType << std::endl;
				break;
			}
			std::cout << data.vertices.size() << "|" << data.indices.size() << "\n";
			dataList.push_back(std::move(data));
		}
		meshDataList.push_back(std::move(dataList));
	}
	else if (node.skin > -1) {
		std::cout << "skin " << node.skin << "\n";
		//TODO
	}
	else if (node.camera > -1) {
		std::cout << "camera " << node.camera << "\n";
		//TODO
	}
	for (const auto childIndex : node.children) {
		std::cout << nodeIndex << " child:";
		if (!loadGltfNode(model, childIndex, scale, meshDataList)) {
			return false;
		}
	}
	return true;
}

bool ModelImport::loadGltf(const char* path, const float scale, const bool isBinary, MeshInput* const meshOut) const {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;//TODO should this be reused?
	loader.SetImageLoader(LoadImageData, nullptr);
	std::string warning, error;
	bool loadResult;
	if (isBinary) {
		loadResult = loader.LoadBinaryFromFile(&model, &warning, &error, path);
	}
	else {
		loadResult = loader.LoadASCIIFromFile(&model, &warning, &error, path);
	}
	if (loadResult == false) {
		std::cout << warning << "\n----\n" << error << std::endl;
		return false;
	}
	if (!warning.empty()) {
		std::cout << warning << std::endl;
	}
	std::cout << "scene count = " << model.scenes.size() << " default = " << model.defaultScene << std::endl;
	auto sceneIndex = model.defaultScene > -1 ? model.defaultScene : 0;
	tinygltf::Scene scene = model.scenes[sceneIndex];
	std::cout << "name = " << scene.name << " node = " << scene.nodes.size() << "\n";
	std::vector<std::vector<VertexIndexed>> meshDataList;
	for (const auto nodeIndex : scene.nodes) {
		if (!loadGltfNode(model, nodeIndex, scale, meshDataList)) {
			return false;
		}
	}
	meshOut->setMesh(std::move(meshDataList));
	std::cout << std::endl;
	return true;
}

bool ModelImport::load(const std::string& path, const float scale, MeshInput* const mesh) const {
	bool result = false;
	if (stringEndsWith(path, ".obj")) {
		return loadObj(path.c_str(), scale, mesh);
	}
	else if (stringEndsWith(path, ".glft")) {
		return loadGltf(path.c_str(), scale, false, mesh);
	}
	else if (stringEndsWith(path, ".glb")) {
		return loadGltf(path.c_str(), scale, true, mesh);
	}
	std::cout << "unknown format" << std::endl;
	return result;
}