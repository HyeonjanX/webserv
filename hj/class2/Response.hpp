#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <fstream>  // ifstream
#include <sstream>  // for ostringstream
#include <string>
#include <map>
#include <sys/stat.h>

class Response
{
private:
    std::string _httpVersion;
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;

private:
    // 데이터 전송 관련
    std::string _data;       // Entire HTTP Response as a string
    size_t _totalBytes;      // Total bytes to be sent
    size_t _sendedBytes; // Bytes sent so far

private:
    std::string _filePath;

public:
    // Constructor
    Response(void);
    Response(const std::string &httpVersion, int statusCode, const std::string &statusMessage);
    virtual ~Response(void);

public:
    
    void init(const std::string &httpVersion, int statusCode, const std::string &statusMessage);
    void clean(void);

    void generateResponseData(void);

public:
    // Getter and Setter for httpVersion
    void setHttpVersion(const std::string &version);
    std::string getHttpVersion() const;

    // Getter and Setter for statusCode
    void setStatusCode(int code);
    int getStatusCode() const;

    // Getter and Setter for statusMessage
    void setStatusMessage(const std::string &message);
    std::string getStatusMessage() const;

    // Getter and Setter for headers
    void setHeader(const std::string &key, const std::string &value);
    bool hasHeader(const std::string &key) const;
    std::string getHeader(const std::string &key) const;

    // Getter and Setter for body
    void setBody(const std::string &b);
    std::string getBody() const;

    std::string getData() const;
    void updateSendedBytes(size_t bytes);
    size_t getTotalBytes() const;
    size_t getSendedBytes() const;

    void setFilePath(const std::string& filePath);
};

#endif