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
#include "File.hpp"

#define BACKLOG 128

class Server;
class EventHandler;
class Webserver;
class Request;

#include "Webserver.hpp"
#include "EventHandler.hpp"
#include "Server.hpp"
#include "Request.hpp"

typedef enum CLIENT_STATUS {
  BEFORE_READ,
  READ_REQUESTLINE,
  READ_HEADER,
  READ_BODY,
  BODY_LIMIT_OVER,
  BODY_SIZE_OVER,
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
    static const size_t _bodyLimit = 100000000;

private:
    
    int                 _socket;
    struct sockaddr_in  _addr;

    Request				_request;

    Webserver*          _ws;
    Server*             _server;
    EventHandler*       _eventHandler;

public:
  
    std::string _data;
    std::string _requestLine;
    std::string _header;
    std::vector<std::pair<std::string, std::string> > _header2;
    std::string _body;

    // ssize_t _bytes_read;
    size_t _contentLength;

    int _status;
    int _ischunk;
    int _erron;
    int _defaultBodyNeed;

    std::string _errMessage;

    Response    _response;

public:
    // 레퍼런스와 디폴트값을 함께 사용하지 않기.
    // Client(int serverSocket);
    Client(int serverSocket, Webserver *ws = NULL, Server *s = NULL, EventHandler *e = NULL);
    virtual ~Client(void);

public:
    int readProcess(void);

    void readRequestLine(void);
    void readHeader(void);
    void readBody(void);

    int afterRead(void);

    int notCgiGetProcess(const std::string &filepath);
    int notCgiPostProcess(const std::string &filepath, const std::string &body);
    int notCgiDeleteProcess(const std::string &filepath);
    int readDefaultErrorFile(const std::string &filepath);

    std::string createDefaultPage(int statusCode);
    std::string createDefaultBody(int statusCode);

public:
    int sendProcess(void);
    
    // int readFile(const std::string &filePath); => File로 이동
    void makeResponseData(void);
    int makeResponse(const std::string & filePath);
    void setResponseStatus(int statusCode, const std::string &statusMessage);
    int checkSendBytes() const;
    void cleanRequestReponse(void);
    
    int chunkRead(void);
    void handleHeaders(void);

public:
    int getSocket(void) const;
    std::string& getData(void);
    // size_t getDataLength(void);

    int tempMakeResponseByStatusCode(int statusCode);


};

#endif