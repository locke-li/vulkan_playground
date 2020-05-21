#include "ModelImport.h"
#include "MeshNode.h"
#include "ImageInput.h"
#include "glm.hpp"
#include "gtc/type_ptr.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include <unordered_map>
#include <iostream>

struct LoadingGltfData {
	ModelLoadingInfo& info;
	std::vector<MeshData>& mesh;
};

//this function processes image data embeded in glb/gltf files,
//which are loaded during model loading
bool LoadImageData(tinygltf::Image* image, const int image_idx, std::string* err,
	std::string* warn, int req_width, int req_height,
	const unsigned char* bytes, int size, void* userData) {
	std::cout << image_idx << " " << image->name << "|" << size << std::endl;
	if (!warn->empty()) {
		std::cout << *warn << std::endl;
	}
	if (!err->empty()) {
		std::cout << *err << std::endl;
		return false;
	}
	auto* data = static_cast<LoadingGltfData*>(userData);
	ImageInput imageInput(true, true);
	imageInput.setData(bytes, size, req_width, req_height);
	//TODO use this for testing
	assert(image_idx == data->info.texture.count());
	auto textureIndex = data->info.texture.addTexture(std::move(imageInput));
	//TODO map image_idx to textureIndex
	return true;
}

bool stringEndsWith(const std::string& value, const std::string& suffix) {
	//TODO case insensitive compare
	if (value.length() >= suffix.length()) {
		return value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0;
	}
	return false;
}

///
/// obj format files are loaded as a single buffer, single bufferView mesh
///
bool ModelImport::loadObj(const char* path, ModelLoadingInfo&& info) const {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapeList;
	std::vector<tinyobj::material_t> materialList;
	std::string warning, error;

	if (!tinyobj::LoadObj(&attrib, &shapeList, &materialList, &warning, &error, path)) {
		std::cout << warning << "\n----\n" << error << std::endl;
		return false;
	}
	if (!warning.empty()) {
		std::cout << warning << std::endl;
	}

	info.mesh.reserve(shapeList.size());
	std::unordered_map<Vertex, uint32_t> vertexIndex;
	std::vector<std::vector<uint8_t>> buffer(1);
	MeshData meshData;
	meshData.data.reserve(shapeList.size());
	meshData.parentIndex = -1;
	meshData.matrix = MatrixInput::identity();
	for (const auto& shape : shapeList) {
		std::cout << shape.name << "\n";
		VertexIndexed data;
		for (const auto& indexInfo : shape.mesh.indices) {
			Vertex vertex{};
			auto index = indexInfo.vertex_index * 3;
			vertex.pos = {
				attrib.vertices[index] / info.scale,
				attrib.vertices[index + 1] / info.scale,
				attrib.vertices[index + 2] / info.scale,
			};
			index = indexInfo.texcoord_index * 2;
			vertex.texCoord = {
				attrib.texcoords[index],
				1.0f - attrib.texcoords[index + 1],
			};
			//TODO other attributes

			if (vertexIndex.count(vertex) == 0) {//unique vertex
				auto index = static_cast<uint32_t>(data.vertices.size());
				vertexIndex[vertex] = index;
				data.vertices.push_back(vertex);
				data.indices.push_back(index);
			}
			else {
				data.indices.push_back(vertexIndex[vertex]);
			}
		}
		std::cout << data.vertices.size() << "|" << data.indices.size() << std::endl;
		meshData.data.push_back(std::move(data));
	}
	info.mesh.setMesh({ std::move(meshData) });
	//TODO manage material/texture
	return true;
}

bool loadGltfNodeMesh(const tinygltf::Model& model, const tinygltf::Node& node, MatrixInput&& matrix, const int* parentIndex, LoadingGltfData& loadingData) {
	const auto& mesh = model.meshes[node.mesh];
	MeshData meshData;
	meshData.data.reserve(mesh.primitives.size());
	meshData.matrix = matrix;
	meshData.parentIndex = parentIndex ? *parentIndex : -1;
	for (const auto& primitive : mesh.primitives) {
		//TODO support for mesh without indices
		if (primitive.indices < 0) continue;
		//glTF seems to pack the same attribute values in a continous bufferview
		//which is not a valid vertex array
		//thus we need to fill our Vertex array, this comes with the benefit of selectively choosing the vertex attributes/types to use
		const auto& posIter = primitive.attributes.find("POSITION");
		if (posIter == primitive.attributes.end()) {
			continue;
		}
		const float* texcoord0 = nullptr;
		const auto& accessorPos = model.accessors[posIter->second];
		const auto& bufferViewPos = model.bufferViews[accessorPos.bufferView];
		const auto* pos = reinterpret_cast<const float*>(&model.buffers[bufferViewPos.buffer].data[accessorPos.byteOffset + bufferViewPos.byteOffset]);
		const auto& texcoord0Iter = primitive.attributes.find("TEXCOORD_0");
		if (texcoord0Iter != primitive.attributes.end()) {
			const auto& accessorTexcoord0 = model.accessors[texcoord0Iter->second];
			const auto& bufferViewTexcoord0 = model.bufferViews[accessorTexcoord0.bufferView];
			texcoord0 = reinterpret_cast<const float*>(&model.buffers[bufferViewTexcoord0.buffer].data[accessorTexcoord0.byteOffset + bufferViewTexcoord0.byteOffset]);
		}
		const auto& accessorIndices = model.accessors[primitive.indices];
		const auto& bufferViewIndices = model.bufferViews[accessorIndices.bufferView];
		const void* indices = &model.buffers[bufferViewIndices.buffer].data[accessorIndices.byteOffset + bufferViewIndices.byteOffset];
		VertexIndexed data;
		data.vertices.reserve(accessorPos.count);
		for (auto i = 0; i < accessorPos.count; ++i) {
			Vertex vertex;
			vertex.pos = glm::make_vec3(reinterpret_cast<const float*>(pos + i * 3)) / loadingData.info.scale;
			if (texcoord0) {
				vertex.texCoord = glm::make_vec2(texcoord0 + i * 2);
			}
			data.vertices.push_back(std::move(vertex));
		}

		switch (accessorIndices.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_BYTE: {
			const auto* indices8 = static_cast<const uint8_t*>(indices);
			for (auto i = 0; i < accessorIndices.count; ++i) {
				data.indices.push_back(indices8[i]);
			}
			break;
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
			data.indices.resize(accessorIndices.count);
			memcpy(data.indices.data(), indices, accessorIndices.count * sizeof(uint16_t));
			break;
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
			//TODO check if 32bit index is really needed?
			const auto* indices32 = static_cast<const uint32_t*>(indices);
			for (auto i = 0; i < accessorIndices.count; ++i) {
				data.indices.push_back(indices32[i]);
			}
			break;
		}
		default:
			std::cout << "unsupported index type: " << accessorIndices.componentType << std::endl;
			break;
		}
		std::cout << data.vertices.size() << "|" << data.indices.size() << "   ";
		if (primitive.material > -1) {
			const auto& mat = model.materials[primitive.material];
			std::cout << mat.name << "\n";
		}
		meshData.data.push_back(std::move(data));
	}
	loadingData.mesh.push_back(std::move(meshData));
	return true;
}

bool loadGltfNode(const tinygltf::Model& model, const int nodeIndex, const int* parentIndex, LoadingGltfData& loadingData) {
	const auto& node = model.nodes[nodeIndex];
	glm::vec3* translation{ nullptr };
	glm::quat* rotation{ nullptr };
	glm::vec3* scale{ nullptr };
	glm::mat4* trs{ nullptr };
	if (node.translation.size() == 3) {
		translation = &glm::vec3{ glm::make_vec3(node.translation.data()) };
	}
	if (node.rotation.size() == 4) {
		rotation = &glm::quat{ glm::make_quat(node.rotation.data()) };
	}
	if (node.scale.size() == 3) {
		scale = &glm::vec3{ glm::make_vec3(node.scale.data()) };
	}
	if (node.matrix.size() == 16) {
		//TODO should we decompose this?
		trs = &glm::mat4{ glm::make_mat4(node.matrix.data()) };
	}
	else {
		//TODO calculate TRS matrix
	}
	MatrixInput&& matrix = {
		translation ? *translation / loadingData.info.scale : glm::vec3(0.0f),
		rotation ? *rotation : glm::identity<glm::quat>(),
		scale ? *scale : glm::vec3(1.0f),
		trs ? *trs : glm::mat4{}
	};
	std::cout << nodeIndex << ":" << node.name << " ";
	if (node.mesh > -1) {
		//TODO point to existing mesh data
		std::cout << node.mesh << "|";
		loadGltfNodeMesh(model, node, std::move(matrix), parentIndex, loadingData);
	}
	else if (node.skin > -1) {
		std::cout << "skin " << node.skin << "\n";
		//TODO
	}
	else if (node.camera > -1) {
		std::cout << "camera " << node.camera << "\n";
		//TODO
	}
	int currentIndex = static_cast<int>(loadingData.mesh.size() - 1);
	for (const auto childIndex : node.children) {
		std::cout << nodeIndex << " child:";
		//children flattened
		if (!loadGltfNode(model, childIndex, &currentIndex, loadingData)) {
			return false;
		}
	}
	return true;
}

bool ModelImport::loadGltf(const std::string& path, const bool isBinary, ModelLoadingInfo&& info) const {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;//TODO should this be reused?
	std::vector<MeshData> meshDataList;
	LoadingGltfData loadingData{ info, meshDataList };
	loader.SetImageLoader(LoadImageData, &loadingData);
	std::string warning, error;
	bool loadResult;
	if (isBinary) {
		loadResult = loader.LoadBinaryFromFile(&model, &error, &warning, path);
	}
	else {
		loadResult = loader.LoadASCIIFromFile(&model, &error, &warning, path);
	}
	if (loadResult == false) {
		std::cout << warning << "\n----\n" << error << std::endl;
		return false;
	}
	if (!warning.empty()) {
		std::cout << warning << std::endl;
	}
	std::cout << "scene count = " << model.scenes.size() << " default = " << model.defaultScene << std::endl;
	auto sceneIndex = model.defaultScene > -1 ? model.defaultScene : 0;
	tinygltf::Scene scene = model.scenes[sceneIndex];
	std::cout << "name = " << scene.name << " node = " << scene.nodes.size() << "\n";
	
	for (const auto nodeIndex : scene.nodes) {
		if (!loadGltfNode(model, nodeIndex, nullptr, loadingData)) {
			return false;
		}
	}
	info.mesh.setMesh(std::move(meshDataList));
	std::cout << std::endl;
	return true;
}

bool ModelImport::load(const std::string& path, ModelLoadingInfo&& info) const {
	bool result = false;
	if (stringEndsWith(path, ".obj")) {
		return loadObj(path.c_str(), std::move(info));
	}
	else if (stringEndsWith(path, ".gltf")) {
		return loadGltf(path, false, std::move(info));
	}
	else if (stringEndsWith(path, ".glb")) {
		return loadGltf(path, true, std::move(info));
	}
	std::cout << "unknown format" << std::endl;
	return result;
}