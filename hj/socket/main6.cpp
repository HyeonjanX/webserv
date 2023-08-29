#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <sstream> // for std::ostringstream, 출력 스트림을 문자열에 쓰는 것과 같은 역할
#include <cstring> // for strerror()
#include <cerrno>  // for errno

/**
 * 함수화 조금 끄적여 보기
 */

typedef int sock_fd_t;

struct Server
{
    int port;
    int max_clients;
    sock_fd_t sock;
    struct sockaddr_in addr;
};

struct Client
{
    int port;
    sock_fd_t sock;

    int buffer_size;
    char buffer[1024];

    std::string header;
    std::string data;

    ssize_t bytes_read;
    size_t content_length;
};

Server socketInit(int port, int max_clients)
{

    std::cout << "socketInit() 호출되어 socket을 생성하고 bind하고 listen합니다. port: " << port << std::endl;

    Server s = {};

    s.port = port;
    s.max_clients = max_clients;

    // 소켓 생성
    if ((s.sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    s.addr.sin_family = AF_INET;         // IPv4
    s.addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    s.addr.sin_port = htons(s.port);     // 서버 포트, 호스트 바이트 순서 => 네트워크 순서로

    // 해당 옵션은 socker 와 bind 사이에 위치해야.
    int enable = 1;
    if (setsockopt(s.sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
    {
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }

    // 소켓에 정보 바인드
    if (bind(s.sock, reinterpret_cast<const struct sockaddr *>(&s.addr), sizeof(s.addr)) == -1)
    {
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }

    if (listen(s.sock, s.max_clients) == -1)
    {
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }

    return s;
}

Client clientInit(Server &s)
{
    Client c = {};

    socklen_t addr_size = static_cast<socklen_t>(sizeof(s.addr));
    c.sock = accept(s.sock, reinterpret_cast<struct sockaddr *>(&s.addr), &addr_size);

    // 임시
    c.buffer_size = 1024;
    c.content_length = 4;

    return c;
}

void readClientData(Client &c)
{
    ssize_t bytes_read;

    do
    {
        bytes_read = read(c.sock, c.buffer, c.buffer_size - 1);
        if (bytes_read > 0)
        {
            c.data.append(c.buffer, bytes_read);
            std::cout << "------------------버퍼 데이터------------------" << std::endl;
            std::cout << c.buffer << std::endl;
            std::cout << "--------------------------------------------" << std::endl;

            if (c.header.empty())
            {
                size_t pos;
                // 헤더 읽는 중, 헤더 끝을 못 찾음 => // 계속해서 data에 헤더 데이터를 읽어들임
                if ((pos = c.data.find("\r\n\r\n")) == std::string::npos)
                {
                    continue;
                }
                c.header = c.data.substr(0, pos);
                c.data = c.data.substr(pos + 4);
                std::cout << "클라이언트 입력에서 헤더를 찾았습니다." << std::endl;
            }

            // 바디 체크: content-length를 초과했는지 검사
            if (c.data.length() > c.content_length)
            {
                std::cerr << "바디길이가 정해진 길이를 초과하였습니다." << std::endl;
                break;
            }
            else if (c.data.length() == c.content_length)
            {
                std::cout << "모든 컨텐츠 수신 완료" << std::endl;
                break;
            }
            else
            {
                // 아직 더 수신해야함.
            }
        }
        else if (bytes_read == 0)
        {
            // 클라이언트가 연결을 종료한 경우 => 클라이언트에서 ctrl+c로 연결을 끊으면.
            std::cout << "클라이언트의 연결이 종료되었습니다." << std::endl;
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 타임아웃 초과
                std::cerr << "Timeout occurred." << std::endl;
            }
            else
            {
                // read() 함수 에러
                std::cerr << "Read error: " << strerror(errno) << std::endl;
            }
            break;
        }

    } while (bytes_read > 0);
}

std::string createHeader(const std::vector<std::string> &headers)
{
    std::string combinedHeader;
    for (std::vector<std::string>::const_iterator it = headers.begin();
         it != headers.end();
         ++it)
    {
        combinedHeader += *it;
        combinedHeader += "\r\n"; // 또는 "\r\n"이 필요한 경우
    }

    return combinedHeader;
}

// #include <sstream> // std::ostringstream은 출력 스트림을 문자열에 쓰는 것과 같은 역할
std::string createContentLengthHeader(size_t body_size)
{
    // 대소문자 구분 X, 보통 Pascal-Case 사용
    const std::string base = "Content-Length: ";
    std::ostringstream oss;

    oss << base << body_size;

    return oss.str();
}

void sendDataToClient(Client &c)
{
    std::vector<std::string> headers;

    headers.push_back("HTTP/1.1 200 OK");
    headers.push_back("Content-Type: text/html");
    headers.push_back("Connection: keep-alive");

    std::string body = "<html><body>Hello, World!</body></html>";

    headers.push_back(createContentLengthHeader(body.length()));

    std::cout << "길이" << body.length() << std::endl;

    // \r\n\r\n을 연결자로 잘못 사용해서 2바이트가 크게 되었을때 보게 되었던 로그
    // Excess found in a non pipelined read: excess = 2, size = 39, maxdownload = 39, bytecount = 0
    // 뒤애 2바이트가 짤리게 됨
    std::string response = createHeader(headers) + "\r\n" + body;

    ssize_t bytes_sent = send(c.sock, response.c_str(), response.length(), 0);
    if (bytes_sent == -1)
    {
        std::cerr << "send() failed: " << strerror(errno) << std::endl;
    }
    else if (static_cast<size_t>(bytes_sent) < response.length())
    {
        // 모든 데이터가 전송되지 않았을 경우의 처리
        std::cerr << "Not all data sent. Sent " << bytes_sent << " bytes out of " << response.length() << std::endl;
        // 필요에 따라 추가적인 로직
    }
}

int main()
{

    int port = 8083;
    int max_clients = 5;

    try
    {
        Server s1 = socketInit(port, max_clients);

        Client c1 = clientInit(s1);

        readClientData(c1);

        std::cout << "==============최종 수신 데이터================" << std::endl;
        std::cout << c1.header << std::endl;
        std::cout << std::endl; // "\r\n\r\n" 형상화
        std::cout << c1.data << std::endl;
        std::cout << "==========================================" << std::endl;

        sendDataToClient(c1);

        close(c1.sock);
        close(s1.sock);
    }
    catch (const std::exception &e)
    {
        std::cerr << "에러 발생: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;

}
