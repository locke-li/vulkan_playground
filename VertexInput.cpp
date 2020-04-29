#include "VertexInput.h"

VkVertexInputBindingDescription VertexInput::getBindingDescription() {
	VkVertexInputBindingDescription binding;
	binding.binding = 0;//index
	binding.stride = sizeof(Vertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return binding;
}

std::vector<VkVertexInputAttributeDescription> VertexInput::getAttributeDescription() {
	std::vector<VkVertexInputAttributeDescription> attribute(3);
	auto& attributePos = attribute[0];
	attributePos.binding = 0;
	attributePos.location = 0;
	attributePos.format = VK_FORMAT_R32G32B32_SFLOAT;//vec3
	attributePos.offset = offsetof(Vertex, pos);
	auto& attributeColor = attribute[1];
	attributeColor.binding = 0;
	attributeColor.location = 1;
	attributeColor.format = VK_FORMAT_R32G32B32_SFLOAT;//vec3
	attributeColor.offset = offsetof(Vertex, color);
	auto& attributeTexCoord = attribute[2];
	attributeTexCoord.binding = 0;
	attributeTexCoord.location = 2;
	attributeTexCoord.format = VK_FORMAT_R32G32_SFLOAT;//vec2
	attributeColor.offset = offsetof(Vertex, texCoord);
	return attribute;
}

uint32_t VertexInput::vertexSize() const {
	return static_cast<uint32_t>(sizeof(vertices[0]) * vertices.size());//TODO handle possible overflow
}

uint32_t VertexInput::vertexCount() const {
	return static_cast<uint32_t>(vertices.size());
}

const Vertex* VertexInput::vertexData() const {
	return vertices.data();
}

uint32_t VertexInput::indexSize() const {
	return static_cast<uint32_t>(sizeof(indices[0]) * indices.size());//TODO handle possible overflow
}

uint32_t VertexInput::indexCount() const {
	return static_cast<uint32_t>(indices.size());
}

const uint16_t* VertexInput::indexData() const {
	return indices.data();
}