#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/hash.hpp"
#include <vector>
#include <memory>

class MeshInput;

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

class MeshNode
{
private:
	const MeshInput* root;
	std::unique_ptr<VertexIndexed> data = nullptr;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 localModelMatrix;
	MeshConstant constantData{};
public:
	static VkVertexInputBindingDescription getBindingDescription();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescription();
	static uint32_t getConstantSize();
public:
	MeshNode(const VertexIndexed&& data,
		const glm::vec3& pos = glm::vec3(0.0f),
		const glm::quat& rot = glm::identity<glm::quat>(),
		const glm::vec3& scale = glm::vec3(1.0f));
	//MeshInput(const MeshInput& other) noexcept;
	MeshNode(MeshNode&& other) noexcept;
	void setRoot(const MeshInput* root) noexcept;
	void setData(const VertexIndexed&& data) noexcept;
	uint32_t vertexSize() const;
	uint32_t vertexCount() const;
	const Vertex* vertexData() const;
	uint32_t indexSize() const;
	uint32_t indexCount() const;
	const uint16_t* indexData() const;
	void updateConstantData();
	void updateConstantDataLocal();
	const MeshConstant& getConstantData() const;
};