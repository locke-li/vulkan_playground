#include "RenderingData.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include <chrono>

void RenderingData::updateUniform(const uint32_t width, const uint32_t height) {
	//static trick here
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration<float, std::chrono::seconds::period>(time - startTime).count();

	uniformData.model = glm::rotate(glm::mat4(1.0f), duration * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	uniformData.view = glm::lookAt(glm::vec3(0.0f, -1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	uniformData.proj = glm::perspective(glm::radians(60.0f), width / (float)height, 0.05f, 10.0f);
	//uniformData.proj[1][1] *= -1;//flip y
}

const UniformBufferData& RenderingData::getUniform() const {
	return uniformData;
}

uint32_t RenderingData::getUniformSize() const {
	return sizeof(uniformData);
}