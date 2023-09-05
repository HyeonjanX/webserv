#include <iostream>

#include "Util.hpp"

int main(int ac, char **av)
{
    if (ac >= 2)
    {
        std::string s = std::string(av[1]);

        std::cout << "ltrim: |" << Util::ltrim(s) << "|" << std::endl;
        std::cout << "rtrim: |" << Util::rtrim(s) << "|" << std::endl;
        std::cout << "lrtrim: |" << Util::lrtrim(s) << "|" << std::endl;
    }
    else
    {
        std::string str("look for non-alphabetic characters...");

        std::size_t n = str.find_first_not_of("abcdefghijklmnopqrstuvwxyz ");

        if (n != std::string::npos)
        {
            std::cout << "The first non-alphabetic character is " << str[n];
            std::cout << " at position " << n << '\n';
        }
    }

    std::string now = Util::getDateString();

    std::cout << "현재시각: " << now << std::endl;

    return 0;
}

// ./test "$(cat tab.txt)"
// $(...): 명령어 치환(Command Substitution). 괄호 안의 명령어(cat tab.txt)의 출력을 가져와 명령어 인자로 사용