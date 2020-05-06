#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/hash.hpp"
#include <vector>

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

struct VertexIndexed {
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
};

struct MeshConstant {
	alignas(16)glm::mat4 model;
};

class MeshInput
{
private:
	VertexIndexed data;
	glm::vec3 position;
	glm::vec4 rotation;
	MeshConstant constantData;
public:
	static VkVertexInputBindingDescription getBindingDescription();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescription();
	static uint32_t getConstantSize();
public:
	MeshInput(const VertexIndexed& data, const glm::vec3 pos, const glm::vec4 rot);
	void setPosition(glm::vec3 pos) noexcept;
	const glm::vec3& getPosition() const noexcept;
	void setRotation(glm::vec4 rot) noexcept;
	const glm::vec4& getRotation() const noexcept;
	uint32_t vertexSize() const;
	uint32_t vertexCount() const;
	const Vertex* vertexData() const;
	uint32_t indexSize() const;
	uint32_t indexCount() const;
	const uint16_t* indexData() const;
	void animate(const float rotationSpeed);
	const MeshConstant& getConstantData() const;
};

