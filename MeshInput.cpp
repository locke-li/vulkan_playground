#include "MeshInput.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include <chrono>
#include <iostream>

VkVertexInputBindingDescription MeshInput::getBindingDescription() {
	VkVertexInputBindingDescription binding;
	binding.binding = 0;//index
	binding.stride = sizeof(Vertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return binding;
}

std::vector<VkVertexInputAttributeDescription> MeshInput::getAttributeDescription() {
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
	attributeTexCoord.offset = offsetof(Vertex, texCoord);
	return attribute;
}

uint32_t MeshInput::getConstantSize() {
	return sizeof(MeshConstant);
}

MeshInput::MeshInput(const VertexIndexed& data, const glm::vec3 pos, const glm::vec4 rot)
	: data(data)
	, position(pos)
	, rotation(rot)
{}

void MeshInput::setPosition(glm::vec3 pos) noexcept {
	position = pos;
}
const glm::vec3& MeshInput::getPosition() const noexcept {
	return position;
}
void MeshInput::setRotation(glm::vec4 rot) noexcept {
	rotation = rot;
}
const glm::vec4& MeshInput::getRotation() const noexcept {
	return rotation;
}

uint32_t MeshInput::vertexSize() const {
	return static_cast<uint32_t>(sizeof(data.vertices[0]) * data.vertices.size());//TODO handle possible overflow
}

uint32_t MeshInput::vertexCount() const {
	return static_cast<uint32_t>(data.vertices.size());
}

const Vertex* MeshInput::vertexData() const {
	return data.vertices.data();
}

uint32_t MeshInput::indexSize() const {
	return static_cast<uint32_t>(sizeof(data.indices[0]) * data.indices.size());//TODO handle possible overflow
}

uint32_t MeshInput::indexCount() const {
	return static_cast<uint32_t>(data.indices.size());
}

const uint16_t* MeshInput::indexData() const {
	return data.indices.data();
}

void MeshInput::animate(const float rotationSpeed) {
	//static trick here
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration<float, std::chrono::seconds::period>(time - startTime).count();

	const auto&& translation = glm::translate(glm::mat4(1.0f), position);
	constantData.model = glm::rotate(translation, duration * glm::radians(rotationSpeed), glm::vec3(0.0f, 1.0f, 0.0f));

	/*
	for (auto i = 0; i < 4; ++i) {
		for (auto j = 0; j < 4; ++j) {
			std::cout << constantData.model[i][j] << " ";
		}
		std::cout << std::endl;
	}
	*/
}

const MeshConstant& MeshInput::getConstantData() const {
	return constantData;
}