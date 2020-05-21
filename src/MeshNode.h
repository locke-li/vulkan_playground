#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "MeshStruct.h"
#include <vector>
#include <memory>

class MeshInput;

class MeshNode
{
private:
	const MeshInput* root;
	const MeshNode* parent;
	std::vector<BufferView> view;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 localModelMatrix;
	MeshConstant constantData{};
public:
	static VkVertexInputBindingDescription getBindingDescription();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescription();
	static uint32_t getConstantSize();
public:
	MeshNode(std::vector<BufferView>&& viewIn,
		const MeshNode* parentIn,
		const MatrixInput& matrix = {
			glm::vec3(0.0f),
			glm::identity<glm::quat>(),
			glm::vec3(1.0f)
		}
	);
	MeshNode(const MeshInput&) = delete;
	MeshNode(MeshNode&&) = default;
	void setRoot(const MeshInput* root) noexcept;
	void setParent(const MeshNode* parent) noexcept;
	void setView(std::vector<BufferView>&& view) noexcept;
	const std::vector<BufferView>& getView() const noexcept;
	void updateConstantData();
	void updateConstantDataAsLocal();
	const MeshConstant& getConstantData() const;
};