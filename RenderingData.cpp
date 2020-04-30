#include "RenderingData.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

void RenderingData::updateCamera(const float fov, const float aspectRatio) {
	uniformData.view = glm::lookAt(glm::vec3(0.0f, -1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	uniformData.proj = glm::perspective(glm::radians(fov), aspectRatio, 0.05f, 10.0f);
}

const UniformBufferData& RenderingData::getUniform() const {
	return uniformData;
}

uint32_t RenderingData::getUniformSize() const {
	return sizeof(uniformData);
}