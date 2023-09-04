#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <sys/socket.h>

#include <string>
#include <vector>
#include <map>

#define BACKLOG 128

typedef enum READ_STATUS {
  READ_NORMAL,
  READ_CHUNKED,
} READ_STATUS;

class Client
{

private:
    static const size_t _headaerLimit = 1000;
    static const size_t _bodyLimit = 1000;
    
    int _socket;
    struct sockaddr_in _addr;

    std::string _data;
    std::string _header;
    std::string _body;

    ssize_t _bytes_read;
    size_t _contentLength;

    int _readMode;

public:
    // 레퍼런스와 디폴트값을 함께 사용하지 않기.
    Client(int serverSocket);
    virtual ~Client(void);

public:
    ssize_t read(void);
    ssize_t write(void);

public:
    int getSocket(void) const;
};

#endif