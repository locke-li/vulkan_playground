#include "MeshInput.h"
#include "MeshNode.h"
#include "DebugHelper.hpp"
#include "gtc/matrix_transform.hpp"
#include <chrono>

MeshInput::MeshInput(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale)
	: enabled(true)
	, meshList(std::vector<MeshNode>())
	, position(pos)
	, rotation(rot)
	, scale(scale)
{
	updateConstantData();
}

MeshInput::MeshInput(MeshInput&& other) noexcept
	: enabled(std::move(other.enabled))
	, bufferList(std::move(other.bufferList))
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

void MeshInput::setEnabled(bool value) noexcept {
	enabled = value;
}
bool MeshInput::isEnabled() const noexcept {
	return enabled;
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

BufferView MeshInput::createView(const size_t offset, const VertexIndexed& data) const {
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
	view.materialIndex = data.material;
	return view;
}

void MeshInput::calculateNormal(VertexIndexed& data) const {
	std::vector<glm::vec3> normal(data.vertices.size());
	std::vector<int> count(data.vertices.size());
	for (auto i = 0; i < data.indices.size(); i += 3) {
		auto v0 = data.indices[i];
		auto v1 = data.indices[i + 1];
		auto v2 = data.indices[i + 2];
		auto e01 = data.vertices[v1].pos - data.vertices[v0].pos;
		auto e02 = data.vertices[v2].pos - data.vertices[v0].pos;
		auto faceNormal = glm::normalize(glm::cross(e01, e02));//TODO winding
		normal[v0] += faceNormal;
		normal[v1] += faceNormal;
		normal[v2] += faceNormal;
		++count[v0];
		++count[v1];
		++count[v2];
	}
	for (auto i = 0; i < normal.size(); ++i) {
		data.vertices[i].normal = normal[i] / static_cast<float>(count[i]);
	}
}

void MeshInput::setMesh(VertexIndexed&& meshData) {
	std::vector<uint8_t> buffer;
	auto verticesSize = meshData.vertices.size() * static_cast<uint32_t>(sizeof(Vertex));
	auto indicesSize = meshData.indices.size() * static_cast<uint8_t>(sizeof(uint16_t));
	buffer.resize(verticesSize + indicesSize);
	BufferView view = createView(0, meshData);
	memcpy(buffer.data() + view.vertexOffset, meshData.vertices.data(), view.vertexSize);
	memcpy(buffer.data() + view.indexOffset, meshData.indices.data(), view.indexSize);
	addMesh({ { std::move(view) }, nullptr, MatrixInput::identity() });
	bufferList.clear();
	bufferList.push_back(std::move(buffer));
}

void MeshInput::setMesh(std::vector<MeshData>&& meshDataList) {
	//we try to keep the number of buffer as low as possible
	//TODO is there an occasion where multiple data source buffer is needed?
	std::vector<uint8_t> buffer;
	size_t offset = 0;
	auto vertexStride = static_cast<uint32_t>(sizeof(Vertex));
	auto indexStride = static_cast<uint8_t>(sizeof(uint16_t));
	for (const auto& meshData : meshDataList) {
		for (const auto& data : meshData.data) {
			auto verticesSize = data.vertices.size() * vertexStride;
			auto indicesSize = data.indices.size() * indexStride;
			offset += verticesSize + indicesSize;
		}
	}
	buffer.resize(offset);
	offset = 0;
	for (const auto& meshData : meshDataList) {
		std::vector<BufferView> viewList;
		viewList.reserve(meshData.data.size());
		for (const auto& data : meshData.data) {
			BufferView view = createView(offset, data);
			offset += view.vertexSize + view.indexSize;
			memcpy(buffer.data() + view.vertexOffset, data.vertices.data(), view.vertexSize);
			memcpy(buffer.data() + view.indexOffset, data.indices.data(), view.indexSize);
			viewList.push_back(std::move(view));
		}
		const auto* parent = (meshData.parentIndex > -1 && meshData.parentIndex < meshList.size()) ? &meshList[meshData.parentIndex] : nullptr;
		addMesh({ std::move(viewList), parent, meshData.matrix });
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