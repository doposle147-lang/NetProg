#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int DEFAULT_ECHO_PORT = 7;
constexpr int BUFFER_SIZE = 1024;

void printUsage(const char* progName) {
    std::cerr << "Использование: " << progName << " <IP-адрес> [порт]\n";
    std::cerr << "По умолчанию используется порт " << DEFAULT_ECHO_PORT << " (TCP echo).\n";
    std::cerr << "Пример для сервера кафедры: " << progName << " 172.16.40.1 7\n";
    std::cerr << "Пример для локального теста: " << progName << " 127.0.0.1 8011\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const char* serverIp = argv[1];
    int port = DEFAULT_ECHO_PORT;

    if (argc >= 3) {
        try {
            port = std::stoi(argv[2]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Ошибка: некорректный номер порта (" << argv[2] << ")\n";
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Ошибка: порт должен быть целым числом\n";
            return 1;
        }
    }

    // Создание TCP сокета
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Ошибка: не удалось создать TCP-сокет (" << strerror(errno) << ")\n";
        return 1;
    }

    // Настройка структуры адреса сервера
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0) {
        std::cerr << "Ошибка: неверный IP-адрес: " << serverIp << "\n";
        close(sock);
        return 1;
    }

    // Установка TCP соединения с сервером
    std::cout << "Попытка подключения к " << serverIp << ":" << port << "...\n";
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка: не удалось подключиться к серверу (" << strerror(errno) << ")\n";
        close(sock);
        return 1;
    }

    std::cout << "Успешно подключено к " << serverIp << ":" << port << std::endl;
    std::cout << "Введите сообщения для отправки на эхо-сервер (введите 'exit' для завершения):" << std::endl;

    std::string line;
    char buffer[BUFFER_SIZE];

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "exit" || line == "quit") {
            std::cout << "Завершение сеанса связи.\n";
            break;
        }

        if (line.empty()) {
            continue;
        }

        // Отправка строки на сервер
        ssize_t bytesSent = send(sock, line.c_str(), line.length(), 0);
        if (bytesSent < 0) {
            std::cerr << "Ошибка при отправке данных (" << strerror(errno) << ")\n";
            break;
        }

        // Прием эхо-ответа от сервера
        ssize_t bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            if (bytesReceived == 0) {
                std::cout << "Сервер закрыл соединение.\n";
            } else {
                std::cerr << "Ошибка при чтении данных (" << strerror(errno) << ")\n";
            }
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << "Эхо: " << buffer << std::endl;
    }

    // Корректное закрытие сокета
    close(sock);
    return 0;
}
