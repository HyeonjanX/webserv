#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <fstream>  // ifstream
#include <sstream>  // for ostringstream
#include <string>
#include <map>
#include <sys/stat.h>

typedef struct s_set_cookie
{
    std::string key;
    std::string value;
    int         expire;

    s_set_cookie(std::string key, std::string value, int expire): key(key), value(value), expire(expire) {}
    s_set_cookie() {}
}   t_set_cookie;

class Response
{
private:
    std::string                         _httpVersion;
    int                                 _statusCode;

    std::map<std::string, std::string>  _headers;
    std::string                         _body;

private:
    // 데이터 전송 관련
    std::string                         _data;          // Entire HTTP Response as a string
    size_t                              _totalBytes;    // Total bytes to be sent
    size_t                              _sendedBytes;   // Bytes sent so far

private:
    std::map<std::string, t_set_cookie>	_cookie;

public:
    // Constructor
    Response(void);
    virtual ~Response(void);

public:
    void clean(void);

    void generateResponseData(void);
    void generate100ResponseData(void);

public:
    // Getter and Setter for httpVersion
    void setHttpVersion(const std::string &version);
    std::string getHttpVersion() const;

    // Getter and Setter for statusCode
    void setStatusCode(int code);
    int getStatusCode() const;

    // Getter and Setter for statusMessage
    void setStatusMessage(const std::string &message);
    std::string getStatusMessage(void) const;

    // Getter and Setter for headers
    void setHeader(const std::string &key, const std::string &value);
    bool hasHeader(const std::string &key) const;
    std::string getHeader(const std::string &key) const;

    // Getter and Setter for body
    void setBody(const std::string &b);
    std::string getBody(void) const;

    std::string getData(void) const;
    // std::string &getData2(void);
    size_t getDataLength(void) const;

    void updateData(size_t bytes);
    void updateSendedBytes(size_t bytes);

    size_t getTotalBytes(void) const;
    size_t getSendedBytes(void) const;

    size_t getTotalBytes(void) { return _totalBytes; }
    size_t getSendBytes(void) { return _sendedBytes; }

    void addCookie(const std::string &key, const std::string &value, size_t expire);
};

#endif