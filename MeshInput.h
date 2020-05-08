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
	VertexIndexed data{};
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	MeshConstant constantData{};
public:
	static VkVertexInputBindingDescription getBindingDescription();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescription();
	static uint32_t getConstantSize();
public:
	MeshInput();
	MeshInput(const glm::vec3& pos);
	MeshInput(const glm::vec3& pos, const glm::quat& rot);
	MeshInput(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale);
	MeshInput(const VertexIndexed&& data, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale);
	void setData(const VertexIndexed&& data) noexcept;
	void setPosition(glm::vec3 pos) noexcept;
	const glm::vec3& getPosition() const noexcept;
	void setRotation(glm::quat rot) noexcept;
	const glm::quat& getRotation() const noexcept;
	uint32_t vertexSize() const;
	uint32_t vertexCount() const;
	const Vertex* vertexData() const;
	uint32_t indexSize() const;
	uint32_t indexCount() const;
	const uint16_t* indexData() const;
	void animate(const float rotationSpeed);
	const MeshConstant& getConstantData() const;
};

