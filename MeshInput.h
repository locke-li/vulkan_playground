#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/hash.hpp"
#include <vector>
#include <memory>

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
	std::unique_ptr<VertexIndexed> data = nullptr;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	MeshConstant constantData{};
public:
	static VkVertexInputBindingDescription getBindingDescription();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescription();
	static uint32_t getConstantSize();
public:
	MeshInput(const VertexIndexed&& data,
		const glm::vec3& pos = glm::vec3(0.0f),
		const glm::quat& rot = glm::identity<glm::quat>(),
		const glm::vec3& scale = glm::vec3(1.0f));
	void setData(const VertexIndexed&& data) noexcept;
	void setPosition(const glm::vec3& pos) noexcept;
	const glm::vec3& getPosition() const noexcept;
	void setRotation(const glm::quat& rot) noexcept;
	const glm::quat& getRotation() const noexcept;
	void setScale(const glm::vec3& scaleIn) noexcept;
	const glm::quat& getScale() const noexcept;
	uint32_t vertexSize() const;
	uint32_t vertexCount() const;
	const Vertex* vertexData() const;
	uint32_t indexSize() const;
	uint32_t indexCount() const;
	const uint16_t* indexData() const;
	void animate(const float rotationSpeed);
	const MeshConstant& getConstantData() const;
};

