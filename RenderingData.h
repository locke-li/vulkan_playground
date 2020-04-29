#pragma once
#include "glm.hpp"

struct UniformBufferData {
	alignas(16)glm::mat4 model;
	alignas(16)glm::mat4 view;
	alignas(16)glm::mat4 proj;
};

class RenderingData
{
private:
	UniformBufferData uniformData;
public:
	void updateUniform(const uint32_t width, const uint32_t height);
	const UniformBufferData& getUniform() const;
	uint32_t getUniformSize() const;
};

