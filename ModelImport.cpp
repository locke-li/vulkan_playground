#include "ModelImport.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include <unordered_map>
#include <iostream>

bool stringEndsWith(const std::string& value, const std::string& suffix) {
	//TODO case insensitive compare
	if (value.length() >= suffix.length()) {
		return value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0;
	}
	return false;
}

bool ModelImport::loadObj(const char* path, const float scale, MeshInput* mesh) const {
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

	std::unordered_map<Vertex, uint32_t> vertexIndex;
	VertexIndexed data;
	for (const auto& shape : shapeList) {
		std::cout << shape.name << "\n";
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
	}
	std::cout << data.vertices.size() << "|" << data.indices.size() << std::endl;
	mesh->setData(std::move(data));
	//TODO manage material/texture
	return true;
}

bool ModelImport::loadGltf(const char* path, const float scale, const bool isBinary, MeshInput* mesh) const {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;//TODO should this be reused?
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
	//TODO currently scene structure ignored
	std::cout << "name = " << scene.name << " node = " << scene.nodes.size() << " |";
	for (const auto& node : scene.nodes) {
		std::cout << node << " ";
	}
	std::cout << "\n";
	std::cout << "bufferView = " << model.bufferViews.size() << "\n";
	for (const auto& view : model.bufferViews) {
		const auto& buffer = model.buffers[view.buffer];
		std::cout << buffer.name << "/" << buffer.data.size() << "\n";
	}
	std::cout << std::endl;
	return true;
}

bool ModelImport::load(const std::string& path, const float scale, MeshInput* mesh) const {
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