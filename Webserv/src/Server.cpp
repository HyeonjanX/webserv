#include "Server.hpp"
#include <stdexcept>

#include <fcntl.h>

Server::Server(int port, const std::vector<t_host> &serverConfig, int sockreuse, int backlog)
    : _port(port), _sockreuse(sockreuse), _backlog(backlog)
{
    for (size_t i = 0; i < serverConfig.size(); i++)
    {
        Host h(*this, serverConfig[i]);
        _hosts.push_back(h);
        
    }
    socketInit(_port, _addr, _sockreuse);
    initListen(_backlog);
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

	// Q. sockreuse를 따로 설정해야 하는 필요가 있을까? (밑에 fcntl과 겹치는데)
    if (sockreuse == 1 && setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &sockreuse, sizeof(int)) < 0)
    {
        throw("setsockopt(SO_REUSEADDR) for server failed");
    }

    if (bind(_socket, (struct sockaddr *)&_addr, sizeof(_addr)) == -1)
    {
        throw("bind error");
    }
}

void Server::initListen(int backlog)
{
    if (fcntl(_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC) == -1)
    {
        throw("fcntl() error");
    }

    if (listen(_socket, backlog) == -1)
    {
        throw("listen error");
    }
}

int Server::getSocket(void) const { return _socket; }
int Server::getPort(void) const { return _port; }
const Host* Server::matchHost(const std::string &hostname)
{
    for (std::vector<Host>::const_iterator it = _hosts.begin(); it < _hosts.end(); ++it)
    {
        if (it.base()->getHostname() == hostname)
        {
            return it.base();
        }
    }
    return _hosts.begin().base();
}
const std::vector<Host>&  Server::getHosts(void) const { return _hosts; }

std::ostream &operator<<(std::ostream &os, const Server &server)
{
    const std::vector<Host> hosts = server.getHosts();

    os << "서버(" << server.getPort() << ")" << std::endl;

    for (size_t i = 0; i < hosts.size(); ++i)
    {
        os << hosts[i];
    }
    os << std::endl;
    return os;
}
