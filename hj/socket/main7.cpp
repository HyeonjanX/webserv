#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/event.h>
#include <unistd.h>

#include <sstream> // for std::ostringstream, 출력 스트림을 문자열에 쓰는 것과 같은 역할
#include <cstring> // for strerror()
#include <cerrno>  // for errno

#include <vector>
#include <map> // map 구조체

// 아래는 아마 지워야하는
#include <thread>
#include <chrono>


/**
 * 함수화 조금 끄적여 보기
 */

typedef int sock_fd_t;
typedef int kqueue_fd_t;

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
    struct sockaddr_in addr;

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
        throw "Failed to create socket: " + std::string(strerror(errno));
    }

    s.addr.sin_family = AF_INET;         // IPv4
    s.addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    s.addr.sin_port = htons(s.port);     // 서버 포트, 호스트 바이트 순서 => 네트워크 순서로

    // 해당 옵션은 socker 와 bind 사이에 위치해야.
    int enable = 1;
    if (setsockopt(s.sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
    {
        throw "Failed to set socket options: " + std::string(strerror(errno));
    }

    // 소켓에 정보 바인드
    if (bind(s.sock, reinterpret_cast<const struct sockaddr *>(&s.addr), sizeof(s.addr)) == -1)
    {
        throw "Failed to bind socket: " + std::string(strerror(errno));
    }

    if (listen(s.sock, s.max_clients) == -1)
    {
        throw "Failed to listen on socket: " + std::string(strerror(errno));
    }

    return s;
}
// uintptr_t	ident;	     /*	identifier for this event */
// 	     short     filter;	     /*	filter for event */
// 	     u_short   flags;	     /*	action flags for kqueue	*/
// 	     u_int     fflags;	     /*	filter flag value */
// 	     int64_t   data;	     /*	filter data value */
// 	     void      *udata;	     /*	opaque user data identifier */
// 	     uint64_t  ext[4];	     /*	extensions */

/**
 * EV_SET(kev, ident,	filter,	flags, fflags, data, udata);
 *
 * kevent(int	kq, const struct kevent	*changelist, int nchanges,
     struct	kevent *eventlist, int nevents,
     const struct timespec *timeout);
 *
*/

int updateEvent(kqueue_fd_t kq, sock_fd_t sock, short filter, u_short flags, u_int fflags, int64_t data, void * /*udata*/)
{

    // EV_SET(  &evSet,  sock,  EVFILT_READ,    EV_ADD, 0,      0,      NULL);
    //          kev,    ident,	filter,	        flags,  fflags, data,   udata);
    // kevent(kq, &evSet, 1, NULL, 0, NULL)

    struct kevent e;

    EV_SET(&e, sock, filter, flags, fflags, data, NULL);
    int result = kevent(kq, &e, 1, NULL, 0, NULL);

    if (result == -1)
    {
        throw "Failed to kevent() for updateEvent(): " + std::string(strerror(errno));
    }

    return result;
}

int loadEvents(kqueue_fd_t kq, struct kevent *eventlist, int nevents)
{
    // 임시용
    // struct timespec timeout;
    // timeout.tv_sec = 1;  // 1초
    // timeout.tv_nsec = 0; // 0 나노초
    // int result = kevent(kq, NULL, 0, eventlist, nevents, &timeout);

    // 무한대기
    int result = kevent(kq, NULL, 0, eventlist, nevents, NULL);
    if (result == -1)
    {
        switch (errno)
        {
        case EINTR:
            std::cerr << "EINTR: Interrupted system call. Retrying..." << std::endl;
            result = loadEvents(kq, eventlist, nevents);
            break;
        case ENOMEM:
            throw "ENOMEM: Insufficient memory.";
        case EBADF:
            throw "EBADF: Bad file descriptor.";
        default:
            throw "Unknown error at loadEvents().";
        }
    }

    if (result == 0)
    {
        // time limit expires, 무한 대기라서 발생하지 않아야함!
        // throw "Fail to loadEvents(): Time limit expires.";
        std::cout << "Fail to loadEvents(): Time limit expires." << std::endl;
        return result;
    }

    //  result: eventlist에 리턴된 감지된 이벤트 갯수, 0 < result <= nevents
    return result;
}

kqueue_fd_t kqueueInit()
{
    kqueue_fd_t kq = kqueue();

    if (kq == -1)
    {
        throw "Failed to kqueue(): " + std::string(strerror(errno));
    }

    return kq;
}

Client clientInit(Server &s)
{
    Client c = {};

    socklen_t addr_size = static_cast<socklen_t>(sizeof(c.addr));
    c.sock = accept(s.sock, reinterpret_cast<struct sockaddr *>(&c.addr), &addr_size);

    // 임시
    c.buffer_size = 1024;
    c.content_length = 4;

    return c;
}

int readClientData(Client &c)
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
                std::cerr << "바디길이(" << c.data.length() << ") 가 정해진 길이()" << c.content_length << ")를 초과하였습니다." << std::endl;
                std::cerr << "컨텐츠: |" << c.content_length << "|" << std::endl;
                return -1;
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
            return 1;
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
            return -1;
        }

    } while (bytes_read > 0);

    return 0;
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
    else if (static_cast<size_t>(bytes_sent) == response.length())
    {
        // 송신 끝.
        std::cout << "전송 완료" << std::endl;
    }
    else
    {
        std::cout << "여긴 어디지?" << std::endl;
    }
}

int main()
{
    int max_clients = 5;
    int nservers = 3;
    // Server  servers[3];
    std::vector<Server> servers;

    std::map<int, Client> clients;

    try
    {
        // config 읽어서 => 그에 따라 서버 생성
        servers.push_back(socketInit(8081, max_clients));
        servers.push_back(socketInit(8082, max_clients));
        servers.push_back(socketInit(8083, max_clients));

        // 비동기 코드 트라이
        kqueue_fd_t kq = kqueueInit();
        for (int i = 0; i < nservers; i++)
        {
            updateEvent(kq, servers[i].sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
        }

        while (true)
        {
            std::cout << "**********************루프 1초후 시작합니다.**********************" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            struct kevent evList[10];
            int nevents = loadEvents(kq, evList, 10);

            std::cout << "nevents: " << nevents << std::endl;

            for (int i = 0; i < nevents; ++i)
            {

                // 아직은 모두~~~ READ 이벤트

                std::vector<Server>::iterator sit;
                for (sit = servers.begin(); sit != servers.end(); ++sit)
                {
                    if (sit->sock == static_cast<sock_fd_t>(evList[i].ident))
                    {
                        break;
                    }
                }

                // 1. 타겟 => 서버 소켓 READ
                if (sit != servers.end())
                {
                    std::cout << "서버 소켓 READ => 클라이언트 소켓 생성" << nevents << std::endl;
                    
                    // 서버 소켓 READ => 클라이언트 소켓 생성 O.K
                    Client client = clientInit(*sit);
                    updateEvent(kq, client.sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    clients.insert(std::make_pair(client.sock, client));
                    continue;
                }

                // 2. 타겟 => 클라이언트 소켓 READ
                int client_sock_fd = evList[i].ident;
                std::map<int, Client>::iterator cit = clients.find(client_sock_fd);

                if (cit == clients.end())
                {
                    // 못찾음(아마 에러?) => 처리 어떻게?
                    continue;
                }

                Client *c = &cit->second;
                
                // 동기적으로 한번에 다 수신
                int result = readClientData(*c);
                // 0 정상수신, -1 에러, 1클라이언트 종료

                if (result == 1)
                {
                    // readClientData 결과1: 클라이언트 종료
                    std::cout << "클라이언트 종료 이후 클린업 1" << std::endl;
                    updateEvent(kq, c->sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                    if (close(c->sock))
                    {
                        std::cerr << "Fail to close." <<
                            " errno: " << errno <<
                            ", msg: " << std::string(strerror(errno)) << std::endl;
                    }
                    std::cout << "클라이언트 종료 이후 클린업 2" << std::endl;
                    clients.erase(c->sock);
                }
                else if (result == -1)
                {
                    // readClientData 결과2: 에러 핸들링
                }
                else
                {
                    // readClientData 결과3: 정상 수신
                    std::cout << "==============최종 수신 데이터================" << std::endl;
                    std::cout << c->header << std::endl;
                    std::cout << std::endl; // "\r\n\r\n" 형상화
                    std::cout << c->data << std::endl;
                    std::cout << "==========================================" << std::endl;

                    // 동기적으로 한번에 다 전송
                    sendDataToClient(*c);
                }
            }
        }

        // 클립업 => 클라이언트 소켓들 정리 && 서버 소켓들 정리
        for (std::map<int, Client>::iterator cit = clients.begin(); cit != clients.end(); ++cit)
        {
            close(cit->second.sock);
        }
        for (std::vector<Server>::iterator sit = servers.begin(); sit != servers.end(); ++sit)
        {
            close(sit->sock);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception occurred: " << e.what() << std::endl;
        return 1;
    }
    catch (int e)
    {
        std::cerr << "integer exception occurred: " << e << std::endl;
    }
    catch (const char *str)
    {
        std::cerr << "string exception occurred: " << str << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception occurred: errno" << errno << ", 내용: " << std::string(strerror(errno)) << std::endl;
    }

    return 0;
}
