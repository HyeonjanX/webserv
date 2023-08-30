#include <string>
#include <algorithm>
#include <iostream>

std::string trimLeadingWhitespace(const std::string &str)
{
    std::string::const_iterator it = str.begin();
    while (it != str.end() && (*it == ' ' || *it == '\t'))
    {
        ++it;
    }
    return std::string(it, str.end());
}

int main()
{
    std::string test = " \t  This is a test";
    std::string result = trimLeadingWhitespace(test);
    
    std::cout << test << std::endl;
    std::cout << result << std::endl;

    return 0;
}