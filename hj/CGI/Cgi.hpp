#ifndef CGI_HPP
#define CGI_HPP

#include <csignal> // for kill
#include <unistd.h> // for close
#include <map>
#include <string>
#include <cstdlib>
#include <iostream>
#include <fcntl.h> // for fcntl

#define READ_FD 0
#define WRITE_FD 1

class Cgi
{
private:
    // Request *_request;
    // Response *_response;
    // Client *_client;
    // EventHandler *_eventHandler;

    int                                 _inPipe[2];
    int                                 _outPipe[2];
    int                                 _pid;
    int                                 _status;
    std::map<std::string, std::string>  _env;

public:
    Cgi();
    Cgi(const Cgi &other);
    Cgi &operator=(const Cgi &other);
    ~Cgi();

public:

    void    exec(const std::string &method, const std::string &data);

    void    clearCgi();
    void    clearChild();
    void    clearPipe();
    void    closePipe(int &fd);
};

#endif // CGI_HPP
