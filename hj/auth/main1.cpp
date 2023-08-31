#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

typedef int socket_fd_t;

// 크롬 테스트 OK
int main()
{

    socket_fd_t s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8084);

    int enable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
    {
        throw "SO_REUSEADDR 설정 실패: " + std::string(strerror(errno));
    }

    bind(s, (struct sockaddr *)&address, sizeof(address));

    listen(s, 3);
    std::cout << "서버시작\n" << std::endl;

    struct sockaddr_in address2;
    int addrlen = sizeof(address);
    socket_fd_t c = accept(s, (struct sockaddr *)&address2, (socklen_t *)&addrlen);

    // fcntl(c, F_SETFL, O_NONBLOCK);

    std::string data;
    const int buffer_size = 102400;
    char buffer[buffer_size] = {0};

    std::cout << "읽기 시작1\n" << std::endl;
    ssize_t read_bytes;
    do
    {
        read_bytes = read(c, buffer, buffer_size);
        if (read_bytes != -1)
        {
            data.append(buffer, read_bytes);
        }
        // std::cout << "=========================" << std::endl;
        // std::cout << data << std::endl;
        // std::cout << "=========================" << std::endl;
    } while(data.find("\r\n\r\n") == std::string::npos);

    // read_bytes = read(c, buffer, buffer_size);
    // data.append(buffer, read_bytes);
    
    std::cout << "받은 요청1\n" << std::endl;
    std::cout << data << std::endl;

    std::string resp = std::string(
        "HTTP/1.1 401 Unauthorized\r\n"
        "WWW-Authenticate: Basic realm=\"Example\"\r\n"
        "\r\n"
    );

    const char* response = "HTTP/1.1 401 Unauthorized\r\n"
                           "Content-Length: 0\r\n"
                           "WWW-Authenticate: Basic realm=\"Example\"\r\n"
                           "\r\n";


    std::cout << "쓰기 시작1\n" << std::endl;
    // ssize_t send_bytes = send(c, resp.c_str(), resp.length(), 0);
    ssize_t send_bytes = send(c, response, strlen(response), 0);
    if (send_bytes == -1)
    {
        throw "send -1 실패: " + std::string(strerror(errno));
    }
    // std::cout << "send: " <<  send_bytes << "resp: " << resp.length() << std::endl;
    std::cout << "send: " <<  send_bytes << "resp: " << strlen(response) << std::endl;

    // sleep (10);
    std::cout << "대기 시작2\n" << std::endl;

    // sleep이 없으면 너무 일찍 read가 호출되어 Operation timed out 되며 연결이 끊긴다.
    sleep(5);
    
    std::string data2;

    do
    {
        read_bytes = read(c, buffer, buffer_size);
        if (read_bytes <= 0)
        {   // Operation timed out
            std::cerr << "read 실패: " << read_bytes << "errno: " << errno << ", " << std::string(strerror(errno));
            throw "예외 던짐";
        }
        data2.append(buffer, read_bytes);
    } while(data2.find("\r\n\r\n") == std::string::npos);

    std::cout << "받은 요청1\n" << std::endl;
    std::cout << data2 << std::endl;

    const char* response2 = "HTTP/1.1 200 OK\r\n"
                           "Content-Length: 2\r\n"
                           "\r\n"
                           "ok";

    ssize_t send_bytes2 = send(c, response2, strlen(response2), 0);
    if (send_bytes2 == -1)
    {
        throw "send -1 실패: " + std::string(strerror(errno));
    }
    // std::cout << "send: " <<  send_bytes << "resp: " << resp.length() << std::endl;
    std::cout << "send: " <<  send_bytes2 << "resp: " << strlen(response2) << std::endl;

    close(c);
    close(s);

    return 0;
}
