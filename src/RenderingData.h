#pragma once
#include "glm.hpp"
#include <vector>
class MeshInput;
class ImageInput;
class MaterialInput;
struct MaterialPrototype;

struct MatrixUniformBufferData {
	alignas(16)glm::mat4 view;
	alignas(16)glm::mat4 proj;
};

struct LightUniformBufferData {
	glm::vec4 debugOption;//reserved for debug
	glm::vec4 cameraPos;//(xyz:pos, w:)
	glm::vec4 lightPos;//(xyz:pos/dir, w:type)
	glm::vec4 lightData;//(x:intensity, y:falloff, z:, w:)
};

enum class LightType : uint8_t {
	Directional = 0,
	Point,
};

struct Light {
	LightType type = LightType::Directional;
	float intensity = 0.0f;
	float falloff = 0.0f;
	glm::vec3 pos;
};

class RenderingData
{
private:
	MatrixUniformBufferData matrixData;
	LightUniformBufferData lightData;
	float cameraFov;
	glm::vec3 cameraPos;
	glm::vec3 cameraViewCenter;
	float windowAspectRatio;
	std::vector<const MeshInput*> renderList;
	std::unordered_set<const MaterialPrototype*> prototypeList;
	std::vector<Light> lightList;
		
	void updateProjection();
	void updateView();
public:
	void setFov(const float fov);
	void setAspectRatio(const float ratio);
	void setPos(const glm::vec3&& pos);
	void setDebugOption(glm::vec4&& debug);
	glm::vec4& getDebugOption();
	void updateCamera(const float fov, const float aspectRatio, const glm::vec3&& pos, const glm::vec3&& center);
	void addLight(Light&& light);
	void updateLight();
	const std::vector<Light>& getLightList() const;
	const MatrixUniformBufferData& getMatrixUniform() const;
	const LightUniformBufferData& getLightUniform() const;
	void setRenderListFiltered(const std::vector<MeshInput>& list);
	const std::vector<const MeshInput*>& getRenderList() const;
	const std::unordered_set<const MaterialPrototype*>& getPrototypeList() const;
};

