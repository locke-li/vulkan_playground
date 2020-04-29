#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "glm.hpp"
#include <vector>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
};

class VertexInput
{
private:
	const std::vector<Vertex> vertices = {
		{{0.0f, -0.577f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
		{{0.0f, 0.289f, 0.577f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{-0.5f, 0.289f, -0.289f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
		{{0.5f, 0.289f, -0.289f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
	};
	const std::vector<uint16_t> indices = {
		0, 1, 2,
		0, 2, 3,
		0, 3, 1,
		3, 2, 1,
	};
public:
	static VkVertexInputBindingDescription getBindingDescription();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescription();
	uint32_t vertexSize() const;
	uint32_t vertexCount() const;
	const Vertex* vertexData() const;
	uint32_t indexSize() const;
	uint32_t indexCount() const;
	const uint16_t* indexData() const;
};

