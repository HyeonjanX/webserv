#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <cerrno>  // for errno
#include <sys/types.h>

#include <cstdlib>
#include <iostream>

#include <string>
#include <vector>
#include <map>

#include <fcntl.h>

#include <sys/stat.h>   // for stat

#include "Response.hpp"
#include "Util.hpp"

#define BACKLOG 128

class Server;
class EventHandler;

typedef enum CLIENT_STATUS {
  BEFORE_READ,
  READ_HEADER,
  READ_BODY,
  READ_END,
  ERROR_400,
  BEFORE_WRITE,
  WRITING,
  C_WRITE,
} CLIENT_STATUS;

class Client
{
public:
    static const size_t _headaerLimit = 1000;
    static const size_t _bodyLimit = 1000;

private:
    
    int                 _socket;
    struct sockaddr_in  _addr;

    Server              _server;
    EventHandler        _eventHandler;

public:
  
    std::string _data;
    std::string _header;
    std::string _body;

    // ssize_t _bytes_read;
    size_t _contentLength;

    int _status;
    int _ischunk;

    std::string _errMessage;

    Response    _response;

public:
    // 레퍼런스와 디폴트값을 함께 사용하지 않기.
    Client(int serverSocket);
    Client(int serverSocket, const &Server s, const &EventHandler e);
    virtual ~Client(void);

public:
    int readProcess(void);



public:
    int sendProcess(void);
    
    int readFile(const std::string &filePath);
    int makeResponse(const std::string & filePath);
    void setResponseStatus(int statusCode, const std::string &statusMessage);
    int checkSendBytes() const;
    void cleanRequestReponse(void);
    
    int chunkRead(void);

public:
    int getSocket(void) const;
    std::string& getData(void);
    std::string& getData(void);
};

#endif