#pragma once
#include <vector>
#include <memory>
#include <string>

class ShaderInput
{
private:
	std::string vertPath;
	std::string fragPath;
	std::unique_ptr<std::vector<char>> vertData;
	std::unique_ptr<std::vector<char>> fragData;
	std::vector<char> loadFile(const std::string& path) const;
public:
	ShaderInput(const std::string& vertexPath, const std::string& fragmentPath);
	std::vector<char> getVertData() const;
	std::vector<char> getFragData() const;
	void preloadVert();
	void preloadFrag();
	void preload();
};

