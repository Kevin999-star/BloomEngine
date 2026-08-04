#pragma once
#include <string>

class Logger {
public:
    static void Log(const std::string& level, const std::string& msg);
};

// Macros del logger
#define LOG_INFO(msg)  Logger::Log("INFO", msg)
#define LOG_WARN(msg)  Logger::Log("WARN", msg)
#define LOG_ERROR(msg) Logger::Log("ERROR", msg)
