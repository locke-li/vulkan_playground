#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include "glm.hpp"
#include "gtc/quaternion.hpp"
#include <vector>

class MeshNode;

class MeshInput
{
private:
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 modelMatrix;
	std::vector<MeshNode> meshList;
public:
	MeshInput(
		const glm::vec3& pos = glm::vec3(0.0f),
		const glm::quat& rot = glm::identity<glm::quat>(),
		const glm::vec3& scale = glm::vec3(1.0f));
	void setPosition(const glm::vec3& pos) noexcept;
	const glm::vec3& getPosition() const noexcept;
	void setRotation(const glm::quat& rot) noexcept;
	const glm::quat& getRotation() const noexcept;
	void setScale(const glm::vec3& scaleIn) noexcept;
	const glm::quat& getScale() const noexcept;
	const glm::mat4& getModelMatrix() const;
	const std::vector<MeshNode>& getMeshList() const;
	void reserve(size_t size);
	void addMesh(MeshNode&& mesh);
	void updateConstantData();
	void animate(const float rotationSpeed);
};

