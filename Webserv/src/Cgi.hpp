#ifndef CGI_HPP
#define CGI_HPP

#include <csignal> // for kill
#include <unistd.h> // for close
#include <map>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <fcntl.h> // for fcntl

#define READ_FD 0
#define WRITE_FD 1
#define READ_BUFFER_SIZE 1000000

#include "Request.hpp"

class Cgi
{
private:

    int                                 _inPipe[2];
    int                                 _outPipe[2];
    int                                 _pid;
    int                                 _status;
    std::vector<std::string>            _env;

    std::string                         _readData;
    std::string                         _postData;
    size_t                              _sendBytes;

public:
    Cgi();
    Cgi(const Cgi &other);
    Cgi &operator=(const Cgi &other);
    ~Cgi();

public:
    int                 &getInPipe(int idx);
    int                 &getOutPipe(int idx);
    const std::string   &getReadData();
    bool                allSend();                 

    const std::string   &getPostData() const;
    void                setPostData(std::string postData);

public:

    void    exec(const std::string &method, const std::string &programPath, const std::vector<std::string> &argv);

    void    clearCgi();
    void    clearChild();
    void    clearPipe();
    void    closePipe(int &fd);

    void    readPipe();
    void    writePipe();

    bool    isPipe(int fd);
    void    pipePrint() const;

    void    setEnvFromRequestHeaders(Request &request, std::string method, std::string path);

	// 신기한 문법?!
    class ExecveException : public std::exception
    {
    public:
        virtual const char *what() const throw();
    };
};

#endif // CGI_HPP
