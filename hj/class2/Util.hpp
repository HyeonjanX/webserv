#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <ctime>
#include <iostream>
#include <sstream>
#include <sys/event.h>

class Util
{

public:
    // trim
    static std::string ltrim(const std::string &str);
    static std::string rtrim(const std::string &str);
    static std::string lrtrim(const std::string &str);
    static std::string getDateString(void);

    static void print_kevent_info(const struct kevent &ke);
    static std::string itoa(size_t value);
};

#endif