#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

constexpr int DEFAULT_DAYTIME_PORT = 13;
constexpr int BUFFER_SIZE = 1024;
constexpr int TIMEOUT_SECONDS = 5;

void printUsage(const char* progName) {
    std::cerr << "Использование: " << progName << " <IP-адрес> [порт]\n";
    std::cerr << "По умолчанию используется порт " << DEFAULT_DAYTIME_PORT << " (UDP daytime).\n";
    std::cerr << "Пример для сервера кафедры: " << progName << " 172.16.40.1 13\n";
    std::cerr << "Пример для локального теста: " << progName << " 127.0.0.1 8013\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const char* serverIp = argv[1];
    int port = DEFAULT_DAYTIME_PORT;

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

    // Создание UDP сокета
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Ошибка: не удалось создать UDP-сокет (" << strerror(errno) << ")\n";
        return 1;
    }

    // Установка тайм-аута ожидания ответа от сервера
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SECONDS;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Предупреждение: не удалось настроить таймаут сокета\n";
    }

    // Заполнение структуры с адресом сервера
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0) {
        std::cerr << "Ошибка: неверный формат IP-адреса: " << serverIp << "\n";
        close(sock);
        return 1;
    }

    // Отправка запроса на сервер
    const char* query = "\n";
    ssize_t sentBytes = sendto(sock, query, std::strlen(query), 0,
                               reinterpret_cast<struct sockaddr*>(&serverAddr),
                               sizeof(serverAddr));
    if (sentBytes < 0) {
        std::cerr << "Ошибка при отправке дейтаграммы (" << strerror(errno) << ")\n";
        close(sock);
        return 1;
    }

    // Ожидание и получение ответа от сервера
    char buffer[BUFFER_SIZE];
    struct sockaddr_in respAddr;
    socklen_t respLen = sizeof(respAddr);

    ssize_t receivedBytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                                     reinterpret_cast<struct sockaddr*>(&respAddr),
                                     &respLen);
    if (receivedBytes < 0) {
        std::cerr << "Ошибка: не получен ответ от сервера (возможно таймаут " 
                  << TIMEOUT_SECONDS << " сек. или служба недоступна)\n";
        close(sock);
        return 1;
    }

    buffer[receivedBytes] = '\0';

    char clientIpStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &respAddr.sin_addr, clientIpStr, sizeof(clientIpStr));

    std::cout << "Ответ от " << clientIpStr << ":" << ntohs(respAddr.sin_port) << ":\n";
    std::cout << buffer;
    if (receivedBytes > 0 && buffer[receivedBytes - 1] != '\n') {
        std::cout << "\n";
    }

    close(sock);
    return 0;
}
