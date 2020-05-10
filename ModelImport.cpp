#include "ModelImport.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
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

	std::unordered_map<Vertex, uint32_t> vertexIndex;
	VertexIndexed data;
	for (const auto& shape : shapeList) {
		//shape.name;
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
	return true;
}

bool ModelImport::loadGltf(const char* path, const float scale, MeshInput* mesh) const {
	//TODO
	return false;
}

bool ModelImport::load(const std::string& path, const float scale, MeshInput* mesh) const {
	bool result = false;
	//TODO check FourCC?
	if (stringEndsWith(path, ".obj")) {
		return loadObj(path.c_str(), scale, mesh);
	}
	else if (stringEndsWith(path, ".glft")) {
		return loadGltf(path.c_str(), scale, mesh);
	}
	std::cout << "unknown format" << std::endl;
	return result;
}