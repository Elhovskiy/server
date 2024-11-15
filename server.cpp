#include "server.h"
#include "data_processor.h"
#include "logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <random>
#include <openssl/sha.h>
#include <cstring>

using T = int16_t;  // Замените на нужный тип, если CODE изменен

// Функция преобразования шестнадцатеричной строки в байтовый массив
void hexStringToBytes(const std::string& hexStr, unsigned char* byteArray, size_t byteArraySize) {
    for (size_t i = 0; i < byteArraySize; ++i) {
        std::stringstream ss;
        ss << std::hex << hexStr.substr(i * 2, 2);
        int byte;
        ss >> byte;
        byteArray[i] = static_cast<unsigned char>(byte);
    }
}

Server::Server(uint16_t port, const std::string& clientDatabaseFile, const std::string& logFilePath)
    : port(port), clientDatabaseFile(clientDatabaseFile), logFilePath(logFilePath) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        logError(logFilePath, "Socket creation failed", true, "Error in socket() function");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        logError(logFilePath, "Setsockopt failed", true, "Error in setsockopt() function");
        exit(EXIT_FAILURE);
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        logError(logFilePath, "Bind failed", true, "Error in bind() function");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        logError(logFilePath, "Listen failed", true, "Error in listen() function");
        exit(EXIT_FAILURE);
    }

    loadDatabase();
}

void Server::loadDatabase() {
    std::ifstream dbFile(clientDatabaseFile);
    if (!dbFile.is_open()) {
        logError(logFilePath, "Failed to open database file", true, clientDatabaseFile);
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(dbFile, line)) {
        std::istringstream iss(line);
        std::string username, password;
        if (std::getline(iss, username, ':') && std::getline(iss, password)) {
            userDatabase[username] = password;
        }
    }
    dbFile.close();
}

void Server::run() {
    std::cout << "Server is listening on port " << port << std::endl;
    sockaddr_in address;
    int addrlen = sizeof(address);

    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            logError(logFilePath, "Client accept failed", false, "Error in accept() function");
            continue;
        }
        handleClient(client_socket);
        close(client_socket);
    }
}

void Server::handleClient(int client_socket) {
    if (!authenticate(client_socket)) {
        return;
    }

    if (!processVectors(client_socket)) {
        const char* errorMsg = "Err: Vector processing failed";
        send(client_socket, errorMsg, strlen(errorMsg), 0);
    }
}

bool Server::authenticate(int client_socket) {
    char username[256] = {0};
    ssize_t bytesRead = read(client_socket, username, sizeof(username) - 1);

    if (bytesRead <= 0) {
        logError(logFilePath, "Failed to read username", false, "Client may have disconnected");
        std::cerr << "Error: Failed to read username or timed out." << std::endl;
        return false;
    }

    username[bytesRead] = '\0';
    std::cout << "Received username: " << username << std::endl;

    if (userDatabase.find(username) == userDatabase.end()) {
        const char* response = "Err: User not found";
        send(client_socket, response, strlen(response), 0);
        std::cerr << "Error: User not found." << std::endl;
        return false;
    }

    // Генерация соли
    std::random_device rd;
    std::mt19937_64 generator(rd());
    uint64_t saltValue = generator();
    
    std::stringstream saltStream;
    saltStream << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << saltValue;
    std::string saltString = saltStream.str();
    std::cout << "Generated SALT: " << saltString << std::endl;

    // Отправка соли клиенту
    ssize_t sentBytes = send(client_socket, saltString.c_str(), saltString.size(), 0);
    if (sentBytes != static_cast<ssize_t>(saltString.size())) {
    	logError(logFilePath, "Failed to send full SALT to client", true, "Incomplete send");
    	std::cerr << "Error: Failed to send full SALT." << std::endl;
    	return false;
	}

    std::cout << "Sent SALT to client: " << saltString << std::endl;

    // Получение хэша от клиента
    char clientHashHex[64 + 1] = {0};  // Ожидаем 64 символа для SHA-256
    bytesRead = read(client_socket, clientHashHex, sizeof(clientHashHex) - 1);

    if (bytesRead != 64) {
        logError(logFilePath, "Failed to read valid hash from client", false, "Invalid hash length");
        std::cerr << "Error: Expected 64 bytes for hash, received " << bytesRead << " bytes." << std::endl;
        return false;
    }
    std::cout << "Received hash from client: " << std::string(clientHashHex, bytesRead) << std::endl;

    // Преобразование хэша клиента в байтовый массив
    unsigned char clientHash[32];
    hexStringToBytes(std::string(clientHashHex, 64), clientHash, 32);

    // Сервер генерирует хэш для проверки
    std::string password = userDatabase[username];
    std::string combined = saltString + password;
    unsigned char generatedHash[32];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.size(), generatedHash);

    // Сравнение с хэшем клиента
    if (memcmp(generatedHash, clientHash, 32) == 0) {
        const char* authResponse = "OK";
        send(client_socket, authResponse, strlen(authResponse), 0);
        std::cout << "Authentication successful. Sent 'OK' to client." << std::endl;
        return true;
    } else {
        const char* authResponse = "Err";
        send(client_socket, authResponse, strlen(authResponse), 0);
        std::cerr << "Authentication failed. Sent 'Err' to client." << std::endl;
        return false;
    }
}

bool Server::processVectors(int client_socket) {
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    uint32_t numVectors = 0;
    ssize_t bytesRead = read(client_socket, &numVectors, sizeof(numVectors));
    if (bytesRead <= 0) {
        logError(logFilePath, "Failed to read number of vectors or timed out", false, "Client may have disconnected");
        return false;
    }
    std::cout << "Number of vectors received: " << numVectors << std::endl;

    std::vector<int16_t> results;
    for (uint32_t i = 0; i < numVectors; ++i) {
        uint32_t vectorSize = 0;
        bytesRead = read(client_socket, &vectorSize, sizeof(vectorSize));
        if (bytesRead <= 0) {
            logError(logFilePath, "Failed to read vector size or timed out", false, "Client may have disconnected");
            return false;
        }
        std::cout << "Size of vector " << i + 1 << ": " << vectorSize << std::endl;

        std::vector<T> vector(vectorSize);
        for (uint32_t j = 0; j < vectorSize; ++j) {
            bytesRead = read(client_socket, &vector[j], sizeof(T));
            if (bytesRead <= 0) {
                logError(logFilePath, "Failed to read vector value", false, "Client may have disconnected");
                return false;
            }
        }

        int16_t result = processVector(vector);
        results.push_back(result);
    }

    // Отправка результатов клиенту
    send(client_socket, results.data(), results.size() * sizeof(int16_t), 0);
    return true;
}

int16_t Server::processVector(const std::vector<T>& vector) {
    // Пример простой обработки вектора
    int16_t result = 0;
    for (const T& value : vector) {
        result += value;
    }
    return result;
}






























