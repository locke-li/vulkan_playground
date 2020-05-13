#include "MeshInput.h"
#include "MeshNode.h"
#include "DebugHelper.hpp"
#include "gtc/matrix_transform.hpp"
#include <chrono>

MeshInput::MeshInput(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale)
	: meshList(std::vector<MeshNode>())
	, position(pos)
	, rotation(rot)
	, scale(scale)
{
	updateConstantData();
}

MeshInput::MeshInput(MeshInput&& other) noexcept
	: meshList(std::move(other.meshList))
	, position(std::move(other.position))
	, rotation(std::move(other.rotation))
	, scale(std::move(other.scale))
	, modelMatrix(std::move(other.modelMatrix))
{
	//TODO better solution?
	for (auto& mesh : meshList) {
		mesh.setRoot(this);
	}
}

void MeshInput::setPosition(const glm::vec3& pos) noexcept {
	position = pos;
}
const glm::vec3& MeshInput::getPosition() const noexcept {
	return position;
}
void MeshInput::setRotation(const glm::quat& rot) noexcept {
	rotation = rot;
}
const glm::quat& MeshInput::getRotation() const noexcept {
	return rotation;
}
void MeshInput::setScale(const glm::vec3& scaleIn) noexcept {
	scale = scaleIn;
}
const glm::vec3& MeshInput::getScale() const noexcept {
	return scale;
}

const glm::mat4& MeshInput::getModelMatrix() const {
	return modelMatrix;
}

const std::vector<MeshNode>& MeshInput::getMeshList() const {
	return meshList;
}

void MeshInput::reserve(size_t size) {
	meshList.reserve(size);
}

void MeshInput::addMesh(MeshNode&& mesh) {
	mesh.setRoot(this);
	mesh.updateConstantData();
	meshList.push_back(std::move(mesh));
}

void MeshInput::updateConstantData() {
	const auto&& translated = glm::translate(glm::mat4(1.0f), position);
	const auto&& rotated = glm::mat4_cast(rotation);
	const auto&& scaled = glm::scale(glm::mat4(1.0f), scale);
	modelMatrix = translated * rotated * scaled;
	for (auto& mesh : meshList) {
		mesh.updateConstantData();
	}
}

void MeshInput::animate(const float rotationSpeed) {
	//static trick here
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration<float, std::chrono::seconds::period>(time - startTime).count();
	rotation = glm::rotate(rotation, glm::radians(-rotationSpeed * 0.01f), glm::vec3(0.0f, -1.0f, 0.0f));

	updateConstantData();
}