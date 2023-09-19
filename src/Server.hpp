#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <sys/socket.h>

#include <string>
#include <vector>
#include <map>

#define BACKLOG 128

#include "Host.hpp"

class Host;
class Location;

class Server
{

private:
    int _socket;
    struct sockaddr_in _addr;

    int _port;

    int _sockreuse;
    int _backlog;

    std::vector<Host> _hosts;

private:
    void socketInit(int port, struct sockaddr_in &addr, int sockreuse);

public:
    // 레퍼런스와 디폴트값을 함께 사용하지 않기.
    Server(int port = 80, int sockreuse = 1, int backlog = 16);
    virtual ~Server(void);

public:
    void initServer(void);
    void initListen(int backlog);
    void initHost(const std::string &hostname);

public:
    int getSocket(void) const;
    int getPort(void) const;
    Host *getHost(const std::string &hostname);
    const std::vector<Host> &getHosts(void) const ;
};

std::ostream &operator<<(std::ostream &os, const Server &server);
#endif