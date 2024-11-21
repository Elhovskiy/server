#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <limits>

constexpr int DEFAULT_PORT = 22852;
constexpr int VECTOR_OVERFLOW_VALUE = -32767;
constexpr int BUFFER_SIZE = 1024;

// Логирование ошибок
void logError(const std::string &logFile, const std::string &level, const std::string &message) {
    std::ofstream outFile(logFile, std::ios::app);
    if (outFile.is_open()) {
        std::time_t now = std::time(nullptr);
        char timeBuffer[64];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        outFile << "[" << timeBuffer << "] [" << level << "] " << message << std::endl;
    }
}

// Генерация случайной соли
std::string generateSalt() {
    uint64_t salt = static_cast<uint64_t>(rand()) << 32 | rand();
    std::stringstream ss;
    ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << salt;
    return ss.str();
}

// Хэширование строки с использованием SHA-256
std::string hashWithSHA256(const std::string &input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.c_str()), input.size(), hash);
    std::stringstream ss;
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Загрузка базы данных клиентов
std::unordered_map<std::string, std::string> loadClientDatabase(const std::string &dbFile) {
    std::unordered_map<std::string, std::string> clients;
    std::ifstream inFile(dbFile);
    if (!inFile.is_open()) {
        throw std::runtime_error("Не удалось открыть файл базы данных клиентов.");
    }
    std::string line;
    while (std::getline(inFile, line)) {
        auto delimiterPos = line.find(':');
        if (delimiterPos != std::string::npos) {
            std::string id = line.substr(0, delimiterPos);
            std::string hash = line.substr(delimiterPos + 1);
            clients[id] = hash;
        }
    }
    return clients;
}

// Класс для работы с векторами
class VectorProcessor {
public:
    // Метод для вычисления суммы элементов вектора
    int16_t computeSum(const std::vector<int16_t>& vector) {
        int32_t sum = 0;  // Используем int32_t для предотвращения переполнения при вычислениях

        for (const auto& elem : vector) {
            sum += elem;  // Суммируем элементы вектора

            // Проверка на переполнение суммы для типа int16_t
            if (sum > std::numeric_limits<int16_t>::max()) {
                return std::numeric_limits<int16_t>::max();  // Переполнение, возвращаем максимально возможное значение
            }

            if (sum < std::numeric_limits<int16_t>::min()) {
                return std::numeric_limits<int16_t>::min();  // Переполнение, возвращаем минимальное значение
            }
        }

        return static_cast<int16_t>(sum);  // Возвращаем результат как int16_t
    }
};


// Обработка клиента
void handleClient(int clientSocket, const std::unordered_map<std::string, std::string> &clients, const std::string &logFile) {
    try {
        char buffer[BUFFER_SIZE];
        // Шаг 2: Получение идентификатора клиента
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) throw std::runtime_error("Ошибка при получении идентификатора.");

        std::string clientID(buffer, bytesReceived);
        std::cout << "Получен идентификатор клиента: " << clientID << std::endl;

        // Шаг 3: Проверка идентификатора и генерация соли
        auto it = clients.find(clientID);
        if (it == clients.end()) {
            send(clientSocket, "ERR", 3, 0);
            throw std::runtime_error("Клиент не найден: " + clientID);
        }
        std::string salt = generateSalt();
        send(clientSocket, salt.c_str(), salt.size(), 0);
        std::cout << "Сгенерированная соль: " << salt << std::endl;

        // Шаг 4: Получение хэша пароля
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) throw std::runtime_error("Ошибка при получении хэша.");

        std::string receivedHash(buffer, bytesReceived);
        std::string expectedHash = hashWithSHA256(salt + it->second);

        // Шаг 5: Проверка хэша
        if (receivedHash != expectedHash) {
            send(clientSocket, "ERR", 3, 0);
            throw std::runtime_error("Ошибка аутентификации клиента: " + clientID);
        }
        send(clientSocket, "OK", 2, 0);

        // Шаги 6-11: Обработка векторов
        uint32_t numVectors = 0;
        recv(clientSocket, &numVectors, sizeof(numVectors), 0);  // Количество векторов
        std::cout << "Получено число векторов: " << numVectors << std::endl;

        VectorProcessor processor; // Инициализация processor

        for (uint32_t i = 0; i < numVectors; ++i) {
            uint32_t vectorSize = 0;
            recv(clientSocket, &vectorSize, sizeof(vectorSize), 0);  // Размер вектора
            std::cout << "Получен размер вектора: " << vectorSize << std::endl;

            std::vector<int16_t> vec(vectorSize);
            size_t bytesReceived = recv(clientSocket, vec.data(), vectorSize * sizeof(int16_t), 0);  // Получение значений вектора
            std::cout << "Ожидаемое количество байт: " << vectorSize * sizeof(int16_t) << ", фактически получено: " << bytesReceived << std::endl;

            if (bytesReceived != vectorSize * sizeof(int16_t)) {
                std::cerr << "Ошибка: получено " << bytesReceived << " байт вместо " << vectorSize * sizeof(int16_t) << std::endl;
                continue;
            }

            std::cout << "Получены значения вектора: ";
            for (auto val : vec) {
                std::cout << val << " ";
            }
            std::cout << std::endl;

            // Вычисление и отправка результата
            int16_t result = processor.computeSum(vec);
            send(clientSocket, &result, sizeof(result), 0);
            std::cout << "Результат для вектора " << i + 1 << ": " << result << std::endl;
        }

    } catch (const std::exception &e) {
        logError(logFile, "ERROR", e.what());
    }
    close(clientSocket);
}


// Главная функция
int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Использование: " << argv[0] << " <client_db_file> <log_file> [port]" << std::endl;
        return EXIT_FAILURE;
    }

    std::string clientDBFile = argv[1];
    std::string logFile = argv[2];
    int port = (argc > 3) ? std::stoi(argv[3]) : DEFAULT_PORT;

    try {
        auto clients = loadClientDatabase(clientDBFile);

        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) throw std::runtime_error("Ошибка создания сокета.");

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
            throw std::runtime_error("Ошибка привязки сокета.");

        if (listen(serverSocket, 5) < 0) throw std::runtime_error("Ошибка запуска прослушивания.");

        std::cout << "Сервер запущен на порту " << port << std::endl;

        while (true) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientLen);
            if (clientSocket < 0) {
                logError(logFile, "WARNING", "Ошибка подключения клиента.");
                continue;
            }
            handleClient(clientSocket, clients, logFile);
        }

        close(serverSocket);

    } catch (const std::exception &e) {
        logError(logFile, "CRITICAL", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}








