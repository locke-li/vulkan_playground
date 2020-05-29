#pragma once
#include "glm.hpp"
#include <iostream>
#include <chrono>

static void log(const glm::mat4& mat) {
	for (auto i = 0; i < 4; ++i) {
		for (auto j = 0; j < 4; ++j) {
			std::cout << mat[i][j] << " ";
		}
		std::cout << "\n";
	}
	std::cout << "-----" << std::endl;
}

static bool logResult(const char* title, bool result) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration<float, std::chrono::milliseconds::period>(time - startTime).count();
	std::cout << "[" << result << "][" << duration << "]" << title << "\n";
	return result;
}