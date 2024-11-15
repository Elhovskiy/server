#include "server.h"
#include "logger.h"
#include <iostream>
#include <string>
#include <cstdlib>

void printHelp() {
    std::cout << "Usage: ./server -c <client_database_file> -l <log_file> [-p <port>]\n";
    std::cout << "Options:\n";
    std::cout << "  -c <client_database_file> : Path to client database file (required)\n";
    std::cout << "  -l <log_file>             : Path to log file (required)\n";
    std::cout << "  -p <port>                 : Port number (optional, default is 22852)\n";
    std::cout << "  -h                        : Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string clientDatabaseFile;
    std::string logFile;
    uint16_t port = 33333; // default port

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" && i + 1 < argc) {
            clientDatabaseFile = argv[++i];
        } else if (arg == "-l" && i + 1 < argc) {
            logFile = argv[++i];
        } else if (arg == "-p" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "-h") {
            printHelp();
            return 0;
        } else {
            printHelp();
            return 1;
        }
    }

    if (clientDatabaseFile.empty() || logFile.empty()) {
        logError(logFile, "Missing required arguments", true, "Missing -c or -l argument");
        printHelp();
        return 1;
    }

    initLogger(logFile);

    try {
        Server server(port, clientDatabaseFile, logFile);
        server.run();
    } catch (const std::exception& e) {
        logError(logFile, "Critical server error", true, e.what());
        return 1;
    }

    return 0;
}

