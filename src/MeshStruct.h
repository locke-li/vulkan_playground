#pragma once
#include "glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/hash.hpp"
#include<vector>

//TODO separate vertex position improves depth prepass/shadow mapping efficiency
struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	bool operator==(const Vertex& other) const;
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(const Vertex& vertex) const {
			return (hash<glm::vec3>()(vertex.pos)) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1) ^
				(hash<glm::vec3>()(vertex.color) >> 1);
		}
	};
}

struct MatrixInput {
	glm::vec3 translation;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 matrix;

	static MatrixInput identity() {
		return {
			glm::vec3(0.0f),
			glm::identity<glm::quat>(),
			glm::vec3(1.0f)
		};
	}
};

struct VertexIndexed {
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
};

struct MeshData {
	int parentIndex;//-1 for none
	MatrixInput matrix;
	std::vector<VertexIndexed> data;
};

struct BufferView {
	int bufferIndex;
	size_t vertexOffset;
	uint32_t vertexSize;
	uint32_t vertexStride;
	uint32_t vertexCount;
	size_t indexOffset;
	uint32_t indexSize;
	uint8_t indexStride;
	uint32_t indexCount;
};

struct MeshConstant {
	alignas(16)glm::mat4 model;
};