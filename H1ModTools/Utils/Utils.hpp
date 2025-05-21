#pragma once

#include <iostream>

#include "Utils/filesystem.hpp"
#include "Utils/json.hpp"

void Load_settings(const std::string& path, nlohmann::json& settings) {
	utils::filesystem::file file = path;
	if (!file.exists()) {
		std::cerr << "Settings file not found: " << path << std::endl;
		return;
	}
	try {
		std::string json_string;
		file.read_string(&json_string);
		settings = nlohmann::json::parse(json_string);
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cerr << "JSON parse error: " << e.what() << std::endl;
	}
}

void Save_settings(const std::string& path, const nlohmann::json& settings) {
	utils::filesystem::file file = path;
	try {
		file.write_string(settings.dump(4));
	}
	catch (const std::exception& e) {
		std::cerr << "Error writing settings: " << e.what() << std::endl;
	}
}