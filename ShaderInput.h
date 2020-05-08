#pragma once
#include <vector>
#include <memory>

class ShaderInput
{
private:
	const char* vertPath;
	const char* fragPath;
	std::unique_ptr<std::vector<char>> vertData;
	std::unique_ptr<std::vector<char>> fragData;
	std::vector<char> loadFile(const char* path) const;
public:
	ShaderInput(const char* vertexPath, const char* fragmentPath);
	std::vector<char> getVertData() const;
	std::vector<char> getFragData() const;
	void preloadVert();
	void preloadFrag();
	void preload();
};

