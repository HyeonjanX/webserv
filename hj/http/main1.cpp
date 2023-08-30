#include <iostream>
#include <sstream>
#include <string>

#include <vector>

std::string trimLeadingWhitespace(const std::string& str) {
    std::string::const_iterator it = str.begin();
    while (it != str.end() && (*it == ' ' || *it == '\t')) {
        ++it;
    }
    return std::string(it, str.end());
}

std::vector<std::string> parseMultiLineHeaders(const std::string &rawHeaders)
{
    std::string::size_type pos = 0;
    std::string::size_type old_pos = 0;

    std::vector<std::string> headers;

    while ((pos = rawHeaders.find("\r\n", old_pos)) != std::string::npos) {
        
        std::string line = rawHeaders.substr(old_pos, pos - old_pos);
        
        if (line[0] == ' ' || line[0] == '\t')
        {
            // 이전 헤더의 연속
            if (headers.empty())
            {
                throw "잘못된 멀티라인 헤더 시도";
            }
            // TODO: 일단 공백 제거해서 추가, 무엇이 올바른지는 나중에^^
            headers.back().append(trimLeadingWhitespace(line));
            std::cout << "update: " << headers.back() << std::endl;
        }
        else if (!line.empty())
        {
            // 새로운 헤더 추가
            headers.push_back(line);
            std::cout << "new: " << headers.back() << std::endl;
        }
        
        old_pos = pos + 2;  // +2 to skip over the "\r\n"
    }

    return headers;
}

// trimLeadingWhitespace && parseMultiLineHeaders 테스트 코드
int main()
{
    std::string rawHeaders =
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Accept-Language: en-US\r\n"
        "                  en;\r\n"
        "                  q=0.9,\r\n"
        "                  fr;\r\n"
        "                  q=0.8\r\n";
    
    std::cout << "========rawHeaders===========" << std::endl;
    std::cout << rawHeaders << std::endl;
    std::cout << "=================================" << std::endl;

    parseMultiLineHeaders(rawHeaders);

    return 0;
}
