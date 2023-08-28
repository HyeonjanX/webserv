#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// typedef unsigned char           __uint8_t;
// typedef __uint8_t               sa_family_t;
// typedef __uint16_t              in_port_t;

// struct sockaddr_in {
// 	__uint8_t       sin_len;
// 	sa_family_t     sin_family;
// 	in_port_t       sin_port;
// 	struct  in_addr sin_addr;
// 	char            sin_zero[8];
// };

// struct in_addr {
// 	in_addr_t s_addr;
// };

// #define AF_INET         2    
// #define INADDR_ANY              (u_int32_t)0x00000000
// #define htons(x)        __DARWIN_OSSwapInt16(x)
// ((__uint16_t)(__builtin_constant_p(x) ? __DARWIN_OSSwapConstInt16(x) : _OSSwapInt16(x)))

typedef int socket_fd_t;

/**
 * 소켓을 하나 열고, 하나의 클라이언트 요청을 받아 응답한다.
 * 최대 1023까지 데이터를 받을 수 있고, 그외 더 무언가를 하지 않는다.
*/
int main()
{

    std::cout << "1-1" << std::endl;
    // 1-1 nginx > server 소켓 생성

    socket_fd_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8082);

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

    std::cout << "2-2" << std::endl;
    // 2-2 클라이언트 요청 읽기

    const int buffer_size = 1024;
    char buffer[buffer_size] = {0};
    read(new_socket, buffer, buffer_size - 1);

    std::cout << "|" << buffer << "|" << std::endl;


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
