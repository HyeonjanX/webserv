#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <ctime>

class Util
{

public:
    // trim
    static std::string ltrim(const std::string &str);
    static std::string rtrim(const std::string &str);
    static std::string lrtrim(const std::string &str);
    static std::string getDateString(void);
};

#endif