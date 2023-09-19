#include "Response.hpp"
#include "Util.hpp"

Response::Response(void)
    : _statusCode(0), _totalBytes(0), _sendedBytes(0) {}

Response::Response(const std::string &httpVersion, int statusCode, const std::string &statusMessage)
    : _httpVersion(httpVersion), _statusCode(statusCode), _statusMessage(statusMessage), _totalBytes(0), _sendedBytes(0) {}

Response::~Response(void) {}

void Response::init(const std::string &httpVersion, int statusCode, const std::string &statusMessage)
{
    _httpVersion = httpVersion;
    _statusCode = statusCode;
    _statusMessage = statusMessage;

    _headers.clear();
    _body.clear();

    _data.clear();
    _totalBytes = 0;
    _sendedBytes = 0;
}
void Response::clean(void)
{
    std::cout << "Response::clean 호출: Response를 비워냅니다!" << std::endl;
    _httpVersion.clear();
    _statusCode = 0;
    _statusMessage.clear();

    _headers.clear();
    _body.clear();

    _data.clear();
    _totalBytes = 0;
    _sendedBytes = 0;

    _filePath.clear();
}

void Response::generateResponseData(void)
{
    const std::string SP(" ");
    const std::string CRLF("\r\n");
    std::ostringstream oss;

    oss << "HTTP/" << _httpVersion << SP 
        << _statusCode << SP
        << Util::getStatusCodeMessage(_statusCode) << CRLF;

    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
    {
        oss << it->first << ": " << it->second << CRLF;
    }
    // std::cout << "응답 생성 확인" << std::endl;

    oss << CRLF;

    // std::cout << "헤더까지: " << std::endl;
    // std::cout << oss.str() << std::endl;
    
    oss << _body;

    _data = oss.str();
    _totalBytes = _data.size();
    _sendedBytes = 0;

    // std::cout << "_body: " << _body << std::endl;
    // std::cout << "===========================" << std::endl;
    // std::cout << _data << std::endl;
    // std::cout << "===========================" << std::endl;
}

void Response::setHttpVersion(const std::string &version)
{
    _httpVersion = version;
}

std::string Response::getHttpVersion() const
{
    return _httpVersion;
}

void Response::setStatusCode(int code) { _statusCode = code; }

int Response::getStatusCode() const { return _statusCode; }

void Response::setStatusMessage(const std::string &message) { _statusMessage = message; }

std::string Response::getStatusMessage() const { return _statusMessage; }

void Response::setHeader(const std::string &key, const std::string &value) { _headers[key] = value; }

bool Response::hasHeader(const std::string &key) const { return _headers.find(key) != _headers.end(); }

std::string Response::getHeader(const std::string &key) const
{
    if (_headers.find(key) != _headers.end())
    {
        return _headers.at(key);
    }
    return "";
}

void Response::setBody(const std::string &b) { _body = b; }

std::string Response::getBody(void) const { return _body; }

std::string Response::getData(void) const { return _data; }
// std::string &Response::getData2(void) { return _data; }

size_t Response::getDataLength(void) const { return _data.size(); }

void Response::updateData(size_t bytes) { _data = _data.substr(bytes); }

void Response::updateSendedBytes(size_t bytes) { _sendedBytes += bytes; }

size_t Response::getTotalBytes(void) const { return _totalBytes; }

size_t Response::getSendedBytes(void) const { return _sendedBytes; }

void Response::setFilePath(const std::string &filePath) { _filePath = filePath; }