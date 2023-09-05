#include "Response.hpp"

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
    std::ostringstream oss;

    oss << "HTTP/" << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }

    oss << "\r\n";

    oss << _body;

    _data = oss.str();
    _totalBytes = _data.size();
    _sendedBytes = 0;
}

void Response::setHttpVersion(const std::string &version)
{
    _httpVersion = version;
}

std::string Response::getHttpVersion() const
{
    return _httpVersion;
}

// Getter and Setter for statusCode
void Response::setStatusCode(int code)
{
    _statusCode = code;
}

int Response::getStatusCode() const
{
    return _statusCode;
}

// Getter and Setter for statusMessage
void Response::setStatusMessage(const std::string &message)
{
    _statusMessage = message;
}

std::string Response::getStatusMessage() const
{
    return _statusMessage;
}

// Getter and Setter for headers
void Response::setHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

bool Response::hasHeader(const std::string &key) const
{
    return _headers.find(key) != _headers.end();
}

std::string Response::getHeader(const std::string &key) const
{
    if (_headers.find(key) != _headers.end())
    {
        return _headers.at(key);
    }
    return "";
}

// Getter and Setter for body
void Response::setBody(const std::string &b)
{
    _body = b;
}

std::string Response::getBody() const
{
    return _body;
}

// Getter for data
std::string Response::getData() const
{
    return _data;
}

// Update sendedBytes
void Response::updateSendedBytes(size_t bytes)
{
    _sendedBytes += bytes;
}

// Getter for totalBytes
size_t Response::getTotalBytes() const
{
    return _totalBytes;
}

// Getter for sendedBytes
size_t Response::getSendedBytes() const
{
    return _sendedBytes;
}

void Response::setFilePath(const std::string &filePath)
{
    _filePath = filePath;
}