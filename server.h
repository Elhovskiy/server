#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <unordered_map>
#include <vector>  // Для работы с векторами
#include <cstdint> // Для использования uint16_t и других типов

class Server {
public:
    // Конструктор
    Server(uint16_t port, const std::string& clientDatabaseFile, const std::string& logFilePath);

    // Основной метод запуска сервера
    void run();

private:
    int server_fd;  // Дескриптор сокета сервера
    uint16_t port;  // Порт сервера
    std::string clientDatabaseFile;  // Путь к файлу с базой данных клиентов
    std::string logFilePath;  // Путь к файлу для логов

    // База данных пользователей
    std::unordered_map<std::string, std::string> userDatabase;

    // Загрузка базы данных пользователей
    void loadDatabase();

    // Обработка клиента
    void handleClient(int client_socket);

    // Аутентификация клиента
    bool authenticate(int client_socket);

    // Обработка векторов (например, int16_t)
    bool processVectors(int client_socket);

    // Обработка вектора с конкретным типом данных
    int16_t processVector(const std::vector<int16_t>& vector);  // Если работа с int16_t
};

#endif // SERVER_H



