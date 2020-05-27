#include "RenderingData.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

void RenderingData::updateProjection() {
	matrixData.proj = glm::perspective(glm::radians(cameraFov), windowAspectRatio, 0.05f, 10.0f);
}

void RenderingData::updateView() {
	matrixData.view = glm::lookAt(cameraPos, cameraViewCenter, glm::vec3(0.0f, 1.0f, 0.0f));
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

void RenderingData::addLight(Light&& light) {
	lightList.push_back(std::move(light));
}

void RenderingData::updateLight() {
	const auto& light = lightList[0];
	lightData.lightPos = glm::vec4(light.pos, light.type);
	lightData.lightData = glm::vec4(light.intensity, light.falloff, 0.0f, 0.0f);
}

const std::vector<Light>& RenderingData::getLightList() const {
	return lightList;
}

const MatrixUniformBufferData& RenderingData::getMatrixUniform() const {
	return matrixData;
}

const LightUniformBufferData& RenderingData::getLightUniform() const {
	return lightData;
}

void RenderingData::setRenderListFiltered(const std::vector<MeshInput>& list) {
	renderList.clear();
	renderList.reserve(list.size());
	//TODO filter/culling
	for (const auto& input : list) {
		if (!input.isEnabled()) continue;
		renderList.push_back(&input);
	}
}

const std::vector<const MeshInput*> RenderingData::getRenderList() const {
	return renderList;
}