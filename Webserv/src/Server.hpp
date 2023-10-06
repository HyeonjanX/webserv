#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <sys/socket.h>

#include <string>
#include <vector>
#include <map>

#define BACKLOG 128

#include "Host.hpp"
#include "Config.hpp"

#define SOCKET_REUSE_MODE 1
#define BACKLOG_VALUE 16

class Host;
class Location;

class Server
{

private:
    int                 _socket;
    struct sockaddr_in  _addr;

    int                 _port;

    int                 _sockreuse;
    int                 _backlog;

    std::vector<Host>   _hosts;

private:
    void socketInit(int port, struct sockaddr_in &addr, int sockreuse);

public:
    Server(int port, const std::vector<t_host> &serverConfig,
            int sockreuse = SOCKET_REUSE_MODE, int backlog = BACKLOG_VALUE);
    virtual ~Server(void);

public:
    void initServer(void);
    void initListen(int backlog);
    // void initHost(const std::string &hostname);

public:
    int getSocket(void) const;
    int getPort(void) const;
    const Host *matchHost(const std::string &hostname);
    const std::vector<Host> &getHosts(void) const ;
};

std::ostream &operator<<(std::ostream &os, const Server &server);
#endif