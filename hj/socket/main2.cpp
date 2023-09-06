#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

typedef int socket_fd_t;

/**
 * 클라이언트의 요청을 계속 읽어 들이는 코드로 업데이트
 * 하지만!!! 다 읽은 후에도, read호출후 기다림이 생겨서 종료가 되지 않다.
 * 클라이언트쪽에서 ctrl+c를 통해 연결을 종료시키면, read가 종료되고 다음 줄이 실행된다.
*/
int main()
{
    std::cout << "1-1" << std::endl;

    // 1-1 nginx > server 소켓 생성
    socket_fd_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8084);

    std::cout << "1-2" << std::endl;

    // 1-2 생성된 server 소켓에 정보 입력
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    std::cout << "1-3" << std::endl;

    // 1-3 해당 소켓 정보를 토대로 커널에 모니터링 요청
    listen(server_fd, 3);

    std::cout << "2-1" << std::endl;

    // 2-1 클라이언트 요청이 들어오면, 특정 클라이언트와의 통신 위한 소켓 생성.
    int addrlen = sizeof(address);
    socket_fd_t new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

    const int buffer_size = 1024;
    char buffer[buffer_size] = {0};
    std::string data;
    ssize_t bytes_read;

    // 2-2 클라이언트 요청 읽기

    do {
        std::cout << "2-2" << std::endl;
        bytes_read = read(new_socket, buffer, buffer_size - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  // 문자열 끝에 NULL 추가
            data += buffer;
            std::cout << "|" << buffer << "|" << std::endl;
        } else if (bytes_read == 0) {
            // 클라이언트가 연결을 종료한 경우 => 클라이언트에서 ctrl+c로 연결을 끊으면.
            std::cout << "Client disconnected." << std::endl;
            break;
        } else {
            // read() 함수 에러
            std::cerr << "Read error: " << strerror(errno) << std::endl;
            break;
        }

    } while (bytes_read > 0);

    std::cout << "|" << data << "|" << std::endl;

    std::cout << "2-3" << std::endl;

    // 2-3 데이터 읽기를 마친 후, 클라이언트에대 응답 보내기
    const char *response = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body>Hello, World!</body></html>";
    send(new_socket, response, strlen(response), 0);

    std::cout << "3" << std::endl;

    // 3 사용된 소켓 정리
    close(new_socket);
    close(server_fd);

    std::cout << "end" << std::endl;

    return 0;
}