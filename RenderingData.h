#pragma once
#include "glm.hpp"

struct UniformBufferData {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class RenderingData
{
private:
	UniformBufferData uniformData;
public:
	void updateUniform(const uint32_t width, const uint32_t height);
	const UniformBufferData& getUniform() const;
};

