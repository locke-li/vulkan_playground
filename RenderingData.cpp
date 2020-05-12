#include "RenderingData.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

void RenderingData::updateProjection() {
	uniformData.proj = glm::perspective(glm::radians(cameraFov), windowAspectRatio, 0.05f, 10.0f);
}

void RenderingData::updateView() {
	uniformData.view = glm::lookAt(cameraPos, cameraViewCenter, glm::vec3(0.0f, 1.0f, 0.0f));
}

void RenderingData::setFov(const float fov) {
	cameraFov = fov;
	updateProjection();
}
void RenderingData::setAspectRatio(const float ratio) {
	windowAspectRatio = ratio;
	updateProjection();
}
void RenderingData::setPos(const glm::vec3&& pos) {
	cameraPos = pos;
	updateView();
}

void RenderingData::updateCamera(const float fov, const float aspectRatio, const glm::vec3&& pos, const glm::vec3&& center) {
	cameraFov = fov;
	cameraPos = pos;
	cameraViewCenter = center;
	windowAspectRatio = aspectRatio;
	updateView();
	updateProjection();
}

const UniformBufferData& RenderingData::getUniform() const {
	return uniformData;
}

uint32_t RenderingData::getUniformSize() const {
	return sizeof(uniformData);
}

void RenderingData::setRenderListFiltered(const std::vector<MeshNode>& list) {
	renderList.clear();
	renderList.reserve(list.size());
	//TODO filter/culling
	for (const auto& mesh : list) {
		renderList.push_back(&mesh);
	}
}

const std::vector<const MeshNode*> RenderingData::getRenderList() const {
	return renderList;
}