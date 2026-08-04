#include "Logger.h"
#include <iostream>

void Logger::Log(const std::string& level, const std::string& msg) {
    std::cout << "[" << level << "] " << msg << std::endl;
}
