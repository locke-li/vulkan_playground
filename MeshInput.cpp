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
	: bufferList(std::move(other.bufferList))
	, meshList(std::move(other.meshList))
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

const uint8_t* MeshInput::bufferData(const int index) const {
	return bufferList[index].data();
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

void MeshInput::setMesh(std::vector<std::vector<VertexIndexed>>&& meshDataList) {
	//we try to keep the number of buffer as low as possible
	//TODO is there an occasion where multiple data source buffer is needed?
	std::vector<uint8_t> buffer;
	size_t offset = 0;
	auto vertexStride = static_cast<uint32_t>(sizeof(Vertex));
	auto indexStride = static_cast<uint8_t>(sizeof(uint16_t));
	for (const auto& meshData : meshDataList) {
		for (const auto& data : meshData) {
			auto verticesSize = data.vertices.size() * vertexStride;
			auto indicesSize = data.indices.size() * indexStride;
			offset += verticesSize + indicesSize;
		}
	}
	buffer.resize(offset);
	offset = 0;
	for (const auto& meshData : meshDataList) {
		std::vector<BufferView> viewList;
		viewList.reserve(meshData.size());
		for (const auto& data : meshData) {
			BufferView view;
			view.bufferIndex = 0;
			view.vertexOffset = offset;
			view.vertexCount = static_cast<uint32_t>(data.vertices.size());
			view.vertexStride = sizeof(data.vertices[0]);
			view.vertexSize = view.vertexCount * view.vertexStride;
			view.indexOffset = offset + view.vertexSize;
			view.indexCount = static_cast<uint32_t>(data.indices.size());
			view.indexStride = sizeof(data.indices[0]);
			view.indexSize = view.indexCount * view.indexStride;
			offset += view.vertexSize + view.indexSize;
			memcpy(buffer.data() + view.vertexOffset, data.vertices.data(), view.vertexSize);
			memcpy(buffer.data() + view.indexOffset, data.indices.data(), view.indexSize);
			viewList.push_back(view);
		}
		addMesh({ std::move(viewList) });
	}
	bufferList.clear();
	bufferList.push_back(std::move(buffer));
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