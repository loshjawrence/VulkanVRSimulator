#pragma once
#include "VulkanApplication.h"
#include <iostream>
#include <stdexcept>

int main() {
	VulkanApplication app;
    try {
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
		std::cout << "Press enter to exit..." << std::cin.get();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}