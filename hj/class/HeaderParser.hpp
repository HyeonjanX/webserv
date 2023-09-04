#ifndef HEADERPARSER_HPP
#define HEADERPARSER_HPP

#include <string>
#include <vector>
#include "Header.hpp"

class HeaderParser {
public:
    std::vector<Header> parseHeaders(const std::string& rawHeaders);
    int parseRequestLine(const std::string& requestLine);
    int validateRequestLine(const std::string& method, const std::string& requestUri, const std::string& httpVersion);
    int validateMethod(const std::string& method);
    int validateRequestUri(const std::string& requestUri);
    int validateHttpVersion(const std::string& httpVersion);

private:
    std::string trimLeadingWhitespace(const std::string& str);
};

#endif // HEADERPARSER_HPP
