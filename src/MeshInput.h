#pragma once
#include "MeshStruct.h"
#include "MeshNode.h"
#include <vector>

class MeshInput
{
private:
	bool enabled;
	std::vector<std::vector<uint8_t>> bufferList;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 modelMatrix;
	std::vector<MeshNode> meshList;
	BufferView createView(const size_t offset, const VertexIndexed& data) const;
	void addMesh(MeshNode&& mesh);
public:
	MeshInput(
		const glm::vec3& pos = glm::vec3(0.0f),
		const glm::quat& rot = glm::identity<glm::quat>(),
		const glm::vec3& scale = glm::vec3(1.0f));
	MeshInput(const MeshInput& other) = delete;
	MeshInput(MeshInput&& other) noexcept;
	void setEnabled(bool value) noexcept;
	bool isEnabled() const noexcept;
	const uint8_t* bufferData(const int index) const;
	void setPosition(const glm::vec3& pos) noexcept;
	const glm::vec3& getPosition() const noexcept;
	void setRotation(const glm::quat& rot) noexcept;
	const glm::quat& getRotation() const noexcept;
	void setScale(const glm::vec3& scaleIn) noexcept;
	const glm::vec3& getScale() const noexcept;
	const glm::mat4& getModelMatrix() const;
	const std::vector<MeshNode>& getMeshList() const;
	void reserve(size_t size);
	void setMesh(VertexIndexed&& meshData);
	void setMesh(std::vector<MeshData>&& meshDataList);
	void updateConstantData();
	void animate(const float rotationSpeed);
};

