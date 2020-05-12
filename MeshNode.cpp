#include "MeshNode.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "DebugHelper.hpp"
#include <chrono>
#include <iostream>

bool Vertex::operator==(const Vertex& other) const {
	return pos == other.pos && texCoord == other.texCoord && color == other.color;
}

VkVertexInputBindingDescription MeshNode::getBindingDescription() {
	VkVertexInputBindingDescription binding;
	binding.binding = 0;//index
	binding.stride = sizeof(Vertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return binding;
}

std::vector<VkVertexInputAttributeDescription> MeshNode::getAttributeDescription() {
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

MeshNode::MeshNode(const VertexIndexed&& data, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale)
	: data(std::make_unique<VertexIndexed>(data))
	, position(pos)
	, rotation(rot)
	, scale(scale)
{
	updateModelView();
}

MeshNode::MeshNode(MeshNode&& other) noexcept {
	data = std::move(other.data);
	position = other.position;
	rotation = other.rotation;
	scale = other.scale;
	constantData = other.constantData;
}

uint32_t MeshNode::getConstantSize() {
	return sizeof(MeshConstant);
}

void MeshNode::setData(const VertexIndexed&& dataIn) noexcept {
	data = std::make_unique<VertexIndexed>(dataIn);
}

void MeshNode::setPosition(const glm::vec3& pos) noexcept {
	position = pos;
}
const glm::vec3& MeshNode::getPosition() const noexcept {
	return position;
}
void MeshNode::setRotation(const glm::quat& rot) noexcept {
	rotation = rot;
}
const glm::quat& MeshNode::getRotation() const noexcept {
	return rotation;
}
void MeshNode::setScale(const glm::vec3& scaleIn) noexcept {
	scale = scaleIn;
}
const glm::quat& MeshNode::getScale() const noexcept {
	return scale;
}

uint32_t MeshNode::vertexSize() const {
	return static_cast<uint32_t>(sizeof(data->vertices[0]) * data->vertices.size());//TODO handle possible overflow
}

uint32_t MeshNode::vertexCount() const {
	return static_cast<uint32_t>(data->vertices.size());
}

const Vertex* MeshNode::vertexData() const {
	return data->vertices.data();
}

uint32_t MeshNode::indexSize() const {
	return static_cast<uint32_t>(sizeof(data->indices[0]) * data->indices.size());//TODO handle possible overflow
}

uint32_t MeshNode::indexCount() const {
	return static_cast<uint32_t>(data->indices.size());
}

const uint16_t* MeshNode::indexData() const {
	return data->indices.data();
}

void MeshNode::animate(const float rotationSpeed) {
	//static trick here
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration<float, std::chrono::seconds::period>(time - startTime).count();
	rotation = glm::rotate(rotation, glm::radians(-rotationSpeed * 0.01f), glm::vec3(0.0f, -1.0f, 0.0f));

	updateModelView();
}

void MeshNode::updateModelView() {
	const auto&& translated = glm::translate(glm::mat4(1.0f), position);
	const auto&& rotated = glm::mat4_cast(rotation);
	const auto&& scaled = glm::scale(glm::mat4(1.0f), scale);

	constantData.model = translated * rotated * scaled;
}

const MeshConstant& MeshNode::getConstantData() const {
	return constantData;
}