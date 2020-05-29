#include "MeshNode.h"
#include "MeshInput.h"
#include "DebugHelper.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
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
	std::vector<VkVertexInputAttributeDescription> attribute(4);
	auto& attributePos = attribute[0];
	attributePos.binding = 0;
	attributePos.location = 0;
	attributePos.format = VK_FORMAT_R32G32B32_SFLOAT;//vec3
	attributePos.offset = offsetof(Vertex, pos);
	auto& attributeNormal = attribute[1];
	attributeNormal.binding = 0;
	attributeNormal.location = 1;
	attributeNormal.format = VK_FORMAT_R32G32B32_SFLOAT;//vec3
	attributeNormal.offset = offsetof(Vertex, normal);
	auto& attributeColor = attribute[2];
	attributeColor.binding = 0;
	attributeColor.location = 2;
	attributeColor.format = VK_FORMAT_R32G32B32_SFLOAT;//vec3
	attributeColor.offset = offsetof(Vertex, color);
	auto& attributeTexCoord = attribute[3];
	attributeTexCoord.binding = 0;
	attributeTexCoord.location = 3;
	attributeTexCoord.format = VK_FORMAT_R32G32_SFLOAT;//vec2
	attributeTexCoord.offset = offsetof(Vertex, texCoord);
	return attribute;
}

MeshNode::MeshNode(std::vector<BufferView>&& viewIn, const MeshNode* parentIn, const MatrixInput& matrix)
	: root(nullptr)
	, parent(parentIn)
	, view(viewIn)
	, position(matrix.translation)
	, rotation(matrix.rotation)
	, scale(matrix.scale)
{
	//TODO use matrix.matrix
	updateConstantDataAsLocal();
}

uint32_t MeshNode::getConstantSize() {
	return sizeof(MeshConstant);
}

void MeshNode::setRoot(const MeshInput* rootIn) noexcept {
	root = rootIn;
}

void MeshNode::setParent(const MeshNode* parentIn) noexcept {
	parent = parentIn;
}

void MeshNode::setView(std::vector<BufferView>&& viewIn) noexcept {
	view = viewIn;
}

const std::vector<BufferView>& MeshNode::getView() const noexcept {
	return view;
}

void MeshNode::updateConstantData() {
	constantData.model = localModelMatrix;
	if (parent != nullptr) {
		//parent node is placed directly before first child in array
		constantData.model = parent->constantData.model * constantData.model;
	}
	else if (root != nullptr) {
		constantData.model = root->getModelMatrix() * constantData.model;
	}
}

void MeshNode::updateConstantDataAsLocal() {
	const auto&& translated = glm::translate(glm::mat4(1.0f), position);
	const auto&& rotated = glm::mat4_cast(rotation);
	const auto&& scaled = glm::scale(glm::mat4(1.0f), scale);
	localModelMatrix = translated * rotated * scaled;
	updateConstantData();
}

const MeshConstant& MeshNode::getConstantData() const {
	return constantData;
}