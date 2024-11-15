#include "logger.h"
#include <fstream>
#include <iostream>
#include <ctime>

std::string logFilePath;

void initLogger(const std::string& logFilePath) {
    ::logFilePath = logFilePath;
}

void logError(const std::string& logFile, const std::string& message, bool critical, const std::string& parameters) {
    std::ofstream logFileStream(logFile, std::ios::app);
    if (!logFileStream.is_open()) {
        std::cerr << "Error opening log file for errors!" << std::endl;
        return;
    }

    std::time_t now = std::time(nullptr);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    logFileStream << "[" << timeStr << "] "
                  << (critical ? "CRITICAL" : "NON-CRITICAL") << " ERROR: "
                  << message << " | Parameters: " << parameters << std::endl;
}

void logResult(uint32_t vectorId, int16_t result) {
    std::ofstream logFileStream(logFilePath, std::ios::app);
    if (logFileStream.is_open()) {
        logFileStream << "Vector ID: " << vectorId << ", Sum: " << result << std::endl;
    } else {
        std::cerr << "Error opening log file for results!" << std::endl;
    }
}
