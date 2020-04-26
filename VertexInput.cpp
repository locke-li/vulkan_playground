#include "VertexInput.h"

VkVertexInputBindingDescription VertexInput::getBindingDescription() {
	VkVertexInputBindingDescription binding;
	binding.binding = 0;//index
	binding.stride = sizeof(Vertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return binding;
}

std::vector<VkVertexInputAttributeDescription> VertexInput::getAttributeDescription() {
	std::vector<VkVertexInputAttributeDescription> attribute(2);
	auto& attributePos = attribute[0];
	attributePos.binding = 0;
	attributePos.location = 0;
	attributePos.format = VK_FORMAT_R32G32_SFLOAT;//vec2
	attributePos.offset = offsetof(Vertex, pos);
	auto& attributeColor = attribute[1];
	attributeColor.binding = 0;
	attributeColor.location = 1;
	attributeColor.format = VK_FORMAT_R32G32B32_SFLOAT;//vec3
	attributeColor.offset = offsetof(Vertex, color);
	return attribute;
}

int VertexInput::vertexSize() const {
	return sizeof(vertices[0]);
}

uint32_t VertexInput::size() const {
	return static_cast<uint32_t>(vertices.size());
}