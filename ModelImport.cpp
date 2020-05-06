#include "ModelImport.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <unordered_map>

bool ModelImport::load(const char* path) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapeList;
	std::vector<tinyobj::material_t> materialList;
	std::string warning, error;

	if (!tinyobj::LoadObj(&attrib, &shapeList, &materialList, &warning, &error, path)) {
		return false;
	}

	std::unordered_map<Vertex, uint32_t> vertexIndex;
	VertexIndexed data;
	for (const auto& shape : shapeList) {
		for (const auto& indexInfo : shape.mesh.indices) {
			Vertex vertex{};
			auto index = indexInfo.vertex_index * 3;
			vertex.pos = {
				attrib.vertices[index],
				attrib.vertices[index + 1],
				attrib.vertices[index + 2],
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
}