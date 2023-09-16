#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <sys/socket.h>

#include <string>
#include <vector>
#include <map>

#define BACKLOG 128

class Server
{

private:
    int _socket;
    struct sockaddr_in _addr;

    int _port;
    std::string _host;

    int _sockreuse;
    int _backlog;

    void socketInit(int port, struct sockaddr_in &addr, int sockreuse);

public:
    // 레퍼런스와 디폴트값을 함께 사용하지 않기.
    Server(int port = 80, const std::string &host = "",
           int sockreuse = 1, int backlog = 16);
    virtual ~Server(void);

public:
    void initServer(void);
    void initListen(int backlog);

public:
    int getSocket(void) const;
    int getPort(void) const;
    std::string getHost(void) const;
};

#endif