#include "HeaderParser.hpp"
#include <iostream>

std::string HeaderParser::trimLeadingWhitespace(const std::string &str)
{
    std::string::const_iterator it = str.begin();
    while (it != str.end() && (*it == ' ' || *it == '\t'))
    {
        ++it;
    }
    return std::string(it, str.end());
}

std::vector<Header> HeaderParser::parseHeaders(const std::string &rawHeaders)
{
    std::string::size_type pos = 0;
    std::string::size_type old_pos = 0;
    std::vector<Header> headers;

    while ((pos = rawHeaders.find("\r\n", old_pos)) != std::string::npos)
    {
        std::string line = rawHeaders.substr(old_pos, pos - old_pos);

        if (line[0] == ' ' || line[0] == '\t')
        {
            if (headers.empty())
            {
                throw "잘못된 멀티라인 헤더 시도";
            }
            std::string value = trimLeadingWhitespace(line);
            headers.back().setValue(headers.back().getValue() + " " + value);
        }
        else if (!line.empty())
        {
            std::string::size_type colon_pos = line.find(":");
            if (colon_pos != std::string::npos)
            {
                std::string name = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                headers.push_back(Header(name, value));
            }
        }

        old_pos = pos + 2;
    }

    return headers;
}
// GET / HTTP/1.1
// > Host: example.com
// > User-Agent: curl/7.64.1
// > Accept: */*
// Method path version
// \r\n으로 끝나는것을 보장.
int HeaderParser::parseRequestLine(const std::string &requestLine)
{
    const char *sp = " ";
    const char *crlf = "\r\n";
    size_t pos = 0;
    size_t old_pos = 0;

    std::string method;
    std::string requestUri;
    std::string httpVersion;
    
    try
    {
        pos = requestLine.find(sp, old_pos);
        
        if (pos == std::string::npos)
        {
            throw "Not enough sp";
        }

        method = requestLine.substr(old_pos, pos - old_pos);
        
        old_pos = pos + 1;
        pos = requestLine.find(sp, old_pos);
        
        if (pos == std::string::npos)
        {
            throw "Not enough sp";
        }

        requestUri = requestLine.substr(old_pos, pos - old_pos);
        
        old_pos = pos + 1;
        pos = requestLine.find(crlf, old_pos);

        httpVersion = requestLine.substr(old_pos, pos - old_pos);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    catch(const char * msg)
    {
        std::cerr << "Invalid requestLine: " << msg << std::endl;
        return -1;
    }

    return 0;
}

int HeaderParser::validateMethod(const std::string& method)
{
    // const std::string availableMethods[3] = { "GET", "HEAD", "POST" };
    const std::string get = std::string("GET");
    const std::string head = std::string("HEAD");
    const std::string post = std::string("POST");

    const std::string put = std::string("PUT");
    const std::string del = std::string("DELELTE");
    const std::string opt = std::string("OPTIONS");
    const std::string con = std::string("CONNECT");
    const std::string trace = std::string("TRACE");

    if (method != get && method != head && method != post)
    {
        if (method == put || method == del || method == opt || method == con || method == trace)
        {
            return 405; // Method Not Allowed;
        }
        return 400; // 400 Bad Request
    }

    return 0;
}

int HeaderParser::validateRequestUri(const std::string& requestUri)
{
    // HTTP/1.1에서는 Request-URI가 반드시 시작이 '/'이어야 함
    if (requestUri.empty() || requestUri[0] != '/') {
        return 1;
    }

    for (size_t i = 0; i < requestUri.size(); ++i) {
        char c = requestUri[i];
        
        // 영문자, 숫자, 몇몇의 특수문자는 허용
        if (isalnum(c) || c == '/' || c == '.' || c == '-' || c == '_' || c == '~' || c == ':') {
            continue;
        } else {
            return 1;
        }
    }

    return 0;
}

int HeaderParser::validateHttpVersion(const std::string& httpVersion)
{
    if (httpVersion.compare(0, 5, "HTTP/") != 0 || httpVersion.size() != 8)
    {
        return 400;
    }

    const std::string versionStr = httpVersion.substr(5, 3);

    if (versionStr.substr(1, 1) != ".")
    {
        return 400;
    }

    if (versionStr == "1.1" || versionStr == "1.0") {
        return;
    }

    char *endPtr;
    double versionVal = std::strtod(versionStr.c_str(), &endPtr);

    if (*endPtr != '\0') {
        return 400;
    }
    return 505; // HTTP_VERSION_NOT_SUPPORTED

    const std::string ver09 = "HTTP/0.9";
    const std::string ver10 = "HTTP/1.0";
    const std::string ver11 = "HTTP/1.1";
    const std::string ver20 = "HTTP/2.0";
    const std::string ver30 = "HTTP/3.0";

    if (httpVersion == ver11)
    {
        return 0;
    }
    if (httpVersion == ver09 || httpVersion == ver10 || httpVersion == ver20 || httpVersion == ver30)
    {
        return 505;
    }

    return 400;
}

int HeaderParser::validateRequestLine(const std::string& method, const std::string& requestUri, const std::string& httpVersion)
{
    int errCode = validateMethod(method) || validateRequestUri(requestUri) || validateHttpVersion(httpVersion);
    return errCode; 
}