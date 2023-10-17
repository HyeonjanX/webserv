#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <iostream>
#include <unistd.h>
#include <stdexcept>

#include "EventHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Util.hpp"
#include "Cgi.hpp"

#include "JsonParser.hpp"
#include "Config.hpp"

#include <map>
#include <set>

// #include <vector> at EventHandler.hpp
class Client;

typedef struct s_session
{
    std::string id;
    int         count;
    std::time_t expirationTime;

    s_session(std::string str, int val, std::time_t time): id(str), count(val), expirationTime(time) {}
    s_session(int val): count(val) {}
    s_session(): count(0), expirationTime(0) {}
}	t_session;

class Webserver
{
private:
    EventHandler                            _eventHandler;
    std::string                             _configPath;
    std::map<int, std::vector<t_host> >     _serverConfigs;
    std::map<int, Server>                   _servers;
    std::map<int, Client>                   _clients;
    std::set<int>                           _closeFds;
    std::map<std::string, t_session>        _sessions;

public:
    Webserver(int ac, const char **av);
    virtual ~Webserver(void);

public:
    void initWebserver(void);
    void runWebserver(void);

private:
    void initServer(int port, const std::vector<t_host> &hosts);
    void initClient(int serverSocket, Server *s = NULL);

public:
    void closeClient(Client &c);

    std::map<int, Client>::iterator     searchClientByPipeFd(int fd);

public:
    std::map<std::string, t_session>    &getSessions(void);
    void                                insertToCloseFds(int fd);
};

#endif