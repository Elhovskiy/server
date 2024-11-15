#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <cstdint>

void initLogger(const std::string& logFilePath);
void logError(const std::string& logFile, const std::string& message, bool critical, const std::string& parameters);

void logResult(uint32_t vectorId, int16_t result);

#endif
