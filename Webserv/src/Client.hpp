#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <cerrno> // for errno
#include <sys/types.h>

#include <cstdlib>
#include <iostream>

#include <string>
#include <vector>
#include <map>

#include <fcntl.h>

#include <sys/stat.h> // for stat

#include "Response.hpp"
#include "Util.hpp"
#include "File.hpp"

#define BACKLOG 128
#define READ_BUFFER_SIZE 1000000

class Server;
class EventHandler;
class Webserver;
class Request;

#include "Webserver.hpp"
#include "EventHandler.hpp"
#include "Server.hpp"
#include "Request.hpp"
#include "Cgi.hpp"

typedef enum CLIENT_STATUS
{
    BEFORE_READ,
    READ_REQUESTLINE,
    READ_HEADER,
    READ_POST_EXPECT_100,
    READ_BODY,
    BODY_LIMIT_OVER,
    BODY_SIZE_OVER,
    READ_END,
    EXEC_CGI,
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
    int _socket;
    struct sockaddr_in _addr;

    Request _request;

    Webserver *_ws;
    Server *_server;
    EventHandler *_eventHandler;

public:
    size_t                  _contentLength;

    int                     _status;
    int                     _ischunk;
    int                     _erron;
    int                     _defaultBodyNeed;

    std::string             _errMessage;

    Response                _response;

    const Host*             _matchedHost;
    const Location*         _matchedLocation;

    Cgi                     _cgi;

public:
    // 레퍼런스와 디폴트값을 함께 사용하지 않기.
    // Client(int serverSocket);
    Client(int serverSocket, Webserver *ws = NULL, Server *s = NULL, EventHandler *e = NULL);
    virtual     ~Client(void);

public:
    int         getSocket(void) const;

public:
    /* ============ 요청을 읽어들이는 역할 수행 ============ */
    void        readProcess(void);
    
    ssize_t     receiveRequest(void);
    void        parseRequest(void);

    void        readRequestLine(void);
    void        readHeader(void);
    void        readBody(void);

public:
    /* ============ 요청에 대해 수행 with 응답 생성 ============ */
    int         doRequest(void);

    int         doNonCgiProcess(const std::string &method);
    int         notCgiGetProcess(const std::string &root, const std::string &filepath, bool autoindex, const std::vector<std::string> &index);
    int         notCgiPostProcess(const std::string &filepath, const std::string &body);
    int         notCgiDeleteProcess(const std::string &filepath);
    
    void        makeResponseData(int statusCode, int defaultBodyNeed); // _response.generateResponseData() 호출
    std::string createDefaultPage(int statusCode);
    std::string createDefaultBody(int statusCode);

public:
    /* ============ 생성된 응답을 보내는 역할 수행 ============ */
    void        sendProcess(void);

    int         checkSendBytes() const;
    void        cleanRequestReponse(void);

public:
    /* ============ 쩌리들 ============ */
    void        chunkRead(void);
    void        handleHeaders(void);

public:
    Request&    getRequest();
    Response&   getResponse();
    Cgi&        getCgi();

public:
    void        cgiProcess(const std::string &method);
    void        makeCgiResponse();
    void        makeCgiErrorResponse();
    bool        isPipe(int fd);
};

#endif