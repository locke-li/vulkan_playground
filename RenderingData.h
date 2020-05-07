#pragma once
#include "glm.hpp"

struct UniformBufferData {
	alignas(16)glm::mat4 view;
	alignas(16)glm::mat4 proj;
};

class RenderingData
{
private:
	UniformBufferData uniformData;
	float cameraFov;
	glm::vec3 cameraPos;
	glm::vec3 cameraViewCenter;
	float windowAspectRatio;

	void updateProjection();
	void updateView();
public:
	void setFov(const float fov);
	void setAspectRatio(const float ratio);
	void setPos(const glm::vec3&& pos);
	void updateCamera(const float fov, const float aspectRatio, const glm::vec3&& pos, const glm::vec3&& center);
	const UniformBufferData& getUniform() const;
	uint32_t getUniformSize() const;
};

