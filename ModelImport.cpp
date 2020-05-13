#include "ModelImport.h"
#include "MeshNode.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include <unordered_map>
#include <iostream>

bool LoadImageData(tinygltf::Image* image, const int image_idx, std::string* err,
	std::string* warn, int req_width, int req_height,
	const unsigned char* bytes, int size, void* userData) {
	std::cout << image_idx << " " << image->name << std::endl;
	//TODO stub
	return true;
}

bool stringEndsWith(const std::string& value, const std::string& suffix) {
	//TODO case insensitive compare
	if (value.length() >= suffix.length()) {
		return value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0;
	}
	return false;
}

bool ModelImport::loadObj(const char* path, const float scale, MeshInput* const mesh) const {
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

	mesh->reserve(shapeList.size());
	std::unordered_map<Vertex, uint32_t> vertexIndex;
	std::vector<std::vector<uint8_t>> buffer(1);
	std::vector<std::vector<Vertex>> verticesList;
	std::vector<std::vector<uint16_t>> indicesList;
	verticesList.reserve(shapeList.size());
	indicesList.reserve(shapeList.size());
	auto vertexStride = static_cast<uint32_t>(sizeof(Vertex));
	auto indexStride = static_cast<uint8_t>(sizeof(uint16_t));
	uint32_t bufferOffset = 0;
	for (const auto& shape : shapeList) {
		std::cout << shape.name << "\n";
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;
		for (const auto& indexInfo : shape.mesh.indices) {
			Vertex vertex{};
			auto index = indexInfo.vertex_index * 3;
			vertex.pos = {
				attrib.vertices[index] / scale,
				attrib.vertices[index + 1] / scale,
				attrib.vertices[index + 2] / scale,
			};
			index = indexInfo.texcoord_index * 2;
			vertex.texCoord = {
				attrib.texcoords[index],
				1.0f - attrib.texcoords[index + 1],
			};

			if (vertexIndex.count(vertex) == 0) {//unique vertex
				auto index = static_cast<uint32_t>(vertices.size());
				vertexIndex[vertex] = index;
				vertices.push_back(vertex);
				indices.push_back(index);
			}
			else {
				indices.push_back(vertexIndex[vertex]);
			}
		}
		std::cout << vertices.size() << "|" << indices.size() << std::endl;
		auto verticesSize = vertices.size() * vertexStride;
		auto indicesSize = indices.size() * indexStride;
		bufferOffset += verticesSize + indicesSize;
		verticesList.push_back(std::move(vertices));
		indicesList.push_back(std::move(indices));
	}
	buffer[0].resize(bufferOffset);
	auto* data = buffer[0].data();
	bufferOffset = 0;
	for (auto i = 0; i < shapeList.size(); ++i) {
		const auto& vertices = verticesList[i];
		const auto& indices = indicesList[i];
		auto verticesSize = static_cast<uint32_t>(vertices.size() * vertexStride);
		auto indicesSize = static_cast<uint32_t>(indices.size() * indexStride);
		memcpy(data + bufferOffset, vertices.data(), verticesSize);
		memcpy(data + bufferOffset + verticesSize, indices.data(), indicesSize);
		mesh->addMesh(MeshNode{ { {
				0,
				bufferOffset,
				verticesSize,
				vertexStride,
				static_cast<uint32_t>(vertices.size()),
				bufferOffset + verticesSize,
				indicesSize,
				indexStride,
				static_cast<uint32_t>(indices.size())
			} } });
		bufferOffset += verticesSize + indicesSize;
	}
	
	mesh->setBuffer(std::move(buffer));
	//TODO manage material/texture
	return true;
}

bool ModelImport::loadGltf(const char* path, const float scale, const bool isBinary, MeshInput* const meshOut) const {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;//TODO should this be reused?
	loader.SetImageLoader(LoadImageData, nullptr);
	std::string warning, error;
	bool loadResult;
	if (isBinary) {
		loadResult = loader.LoadBinaryFromFile(&model, &warning, &error, path);
	}
	else {
		loadResult = loader.LoadASCIIFromFile(&model, &warning, &error, path);
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
	//TODO currently scene structure ignored, as this is only an int value
	std::cout << "name = " << scene.name << " node = " << scene.nodes.size() << " |";
	for (const auto& node : scene.nodes) {
		std::cout << node << " ";
	}
	std::cout << "\n";
	std::cout << "buffer = " << model.buffers.size() << " bufferView = " << model.bufferViews.size() << "\n";
	for (const auto& view : model.bufferViews) {
		if (view.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER) {//indices
			std::cout << "view indices:" << view.name << "/" << view.byteOffset << "+" << view.byteLength << "\n";
		}
		else if (view.target == TINYGLTF_TARGET_ARRAY_BUFFER) {//vertices
			std::cout << "view vertices:" << view.name << "/" << view.byteOffset << "+" << view.byteLength << "\n";
		}
		else {
			std::cout << "view other:" << view.name << "|" << view.target << "/" << view.byteOffset << "+" << view.byteLength << "\n";
		}
	}
	for (const auto& mesh : model.meshes) {
		std::cout << "mesh:" << mesh.name << "/" << mesh.primitives.size() << ", " << mesh.weights.size() << "\n";
		if (mesh.primitives.size() == 0) continue;
		std::vector<BufferView> meshBufferView;
		meshBufferView.reserve(mesh.primitives.size());
		for (const auto& prim : mesh.primitives) {
			if (prim.indices < 0) continue;
			const auto& accessor = model.accessors[prim.indices];
			const auto& view = model.bufferViews[accessor.bufferView];
			meshBufferView.push_back({ 
				view.buffer, 
				static_cast<uint32_t>(accessor.byteOffset), 
				static_cast<uint32_t>(accessor.count), 
				static_cast<uint32_t>(accessor.ByteStride(view)),
				});
		}
		meshOut->addMesh(MeshNode{ std::move(meshBufferView) });
	}
	std::cout << std::endl;
	return true;
}

bool ModelImport::load(const std::string& path, const float scale, MeshInput* const mesh) const {
	bool result = false;
	if (stringEndsWith(path, ".obj")) {
		return loadObj(path.c_str(), scale, mesh);
	}
	else if (stringEndsWith(path, ".glft")) {
		return loadGltf(path.c_str(), scale, false, mesh);
	}
	else if (stringEndsWith(path, ".glb")) {
		return loadGltf(path.c_str(), scale, true, mesh);
	}
	std::cout << "unknown format" << std::endl;
	return result;
}