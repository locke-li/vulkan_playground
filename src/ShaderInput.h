#pragma once
#include <vector>
#include <memory>
#include <string>

class ShaderInput
{
private:
	std::string vertPath;
	std::string fragPath;
	std::vector<char> vertData;
	std::vector<char> fragData;
	bool loadFile(const std::string& path, std::vector<char>& buffer) const;
public:
	ShaderInput(const std::string& vertexPath, const std::string& fragmentPath);
	const std::vector<char>& getVertData() const;
	const std::vector<char>& getFragData() const;
	bool preloadVert();
	bool preloadFrag();
	bool preload();
	void unload();
};

