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
public:
	void updateCamera(const float fov, const float aspectRatio);
	const UniformBufferData& getUniform() const;
	uint32_t getUniformSize() const;
};

