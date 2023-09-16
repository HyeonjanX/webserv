#include "Server.hpp"

#include <fcntl.h>

Server::Server(int port, const std::string &host, int sockreuse, int backlog)
    : _port(port), _host(host), _sockreuse(sockreuse), _backlog(backlog)
{
    initServer();
}

Server::~Server(void) {}

/**
 * 1. int socket(int domain, int type, int protocol);
 * - domain: 프로토콜 체계 결정: AF_INET (IPv4) | AF_INET6 (IPv6),
 *   - AF<->PF 동일하게 사용: P: Protocal, A: Address
 * - type: SOCK_STREAM(TCP) | SOCK_DGRAM (UDP)
 * - protocol: 프로토콜 지정: 0(자동 지정)
 * 2. addr: 바인딩할 주소 정보가 담긴 구조체
 * - sin_family: AF_INET(IPv4)
 * - sin_addr.s_addr: 연결 수락 포트 설정, INADDR_ANY(0.0.0.0)
 * - sin_port: 포트
 *   - htons: 호스트의 데이터 체계에서 네트워크의 것으로
 */
void Server::initServer(void)
{
    socketInit(_port, _addr, _sockreuse); // IPv4, TCP, port, 0.0.0.0
    initListen(_backlog);                 //
}

void Server::socketInit(int port, struct sockaddr_in &addr, int sockreuse)
{
    if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        throw("socket error");
    }
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (sockreuse == 1 && setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &sockreuse, sizeof(int)) < 0)
    {
        throw("setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(_socket, (struct sockaddr *)&_addr, sizeof(_addr)) == -1)
    {
        throw("bind error");
    }
}

void Server::initListen(int backlog)
{
    if (listen(_socket, backlog) == -1)
    {
        throw("listen error");
    }

    if (fcntl(_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC) == -1)
    {
        throw("fcntl() error");
    }
}

int Server::getSocket(void) const { return _socket; }
int Server::getPort(void) const { return _port; }
std::string Server::getHost(void) const { return _host; }
