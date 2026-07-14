#pragma once

#include <iostream>

#include "libs/pfd.h"

inline void log_err(std::string title, std::string str1, bool display = false) {
	std::cerr << "[ ERROR - " << title << "] " << str1 << std::endl;
	if (display) {
		pfd::message(title, str1, pfd::choice::ok, pfd::icon::error);
	}
}

inline void log_info(std::string title, std::string str1, bool display=false) {
	std::cout << "[ INFO - " << title << "] " << str1 << std::endl;
	if (display) {
		pfd::message(title, str1, pfd::choice::ok, pfd::icon::info);
	}
}

inline void log_warn(std::string title, std::string str1, bool display = false) {
	std::cout << "[ WARN - " << title << "] " << str1 << std::endl;
	if (display) {
		pfd::message(title, str1, pfd::choice::ok, pfd::icon::warning);
	}
}