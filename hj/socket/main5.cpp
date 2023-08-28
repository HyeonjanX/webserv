#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

typedef int socket_fd_t;

/**
 * 클라이언트 요청에 타임아웃 설정
*/
int main()
{
    std::cout << "1-1" << std::endl;

    // 1-1 nginx > server 소켓 생성
    socket_fd_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8083);

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

    const int buffer_size = 10;
    char buffer[buffer_size] = {0};
    std::string header;
    std::string data;
    ssize_t bytes_read;
    
    size_t pos;

    size_t content_length = 4; // 임의의 값

    // 5초 타임아웃
    struct timeval timeout;
    timeout.tv_sec = 5;  
    timeout.tv_usec = 0;

    // 타임아웃 설정
    if (setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        std::cerr << "Setsockopt failed: " << strerror(errno) << std::endl;
    }

    // 2-2 클라이언트 요청 읽기

    do {
        std::cout << "2-2" << std::endl;
        bytes_read = read(new_socket, buffer, buffer_size - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  // 문자열 끝에 NULL 추가
            data += buffer;
            std::cout << "|" << buffer << "|" << std::endl;

            if (header.empty()) {
                // 헤더 읽는 중, 헤더 끝을 못 찾음 => // 계속해서 data에 헤더 데이터를 읽어들임
                if ((pos = data.find("\r\n\r\n")) == std::string::npos) {
                    continue;
                }
                header = data.substr(0, pos);
                data = data.substr(pos + 4);
                std::cout << "Found HTTP header end." << std::endl;
            }

            // 바디 체크: content-length를 초과했는지 검사
            if (data.length() > content_length) {
                std::cerr << "Content length error: received more data than expected." << std::endl;
                break;
            } else if (data.length() == content_length) {
                std::cout << "Received all content." << std::endl;
                break;
            } else {
                // 아직 더 수신해야함.
            }
        } else if (bytes_read == 0) {
            // 클라이언트가 연결을 종료한 경우 => 클라이언트에서 ctrl+c로 연결을 끊으면.
            std::cout << "Client disconnected." << std::endl;
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 타임아웃 초과
                std::cerr << "Timeout occurred." << std::endl;
            } else {
                // read() 함수 에러
                std::cerr << "Read error: " << strerror(errno) << std::endl;
            }
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
