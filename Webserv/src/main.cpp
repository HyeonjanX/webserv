#include <iostream>

#include "Util.hpp"
#include "Webserver.hpp"

int main(int ac, const char **av)
{
    try
    {
        Webserver ws(ac, av);

        ws.initWebserver();
        ws.runWebserver();
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "ws runtime_error 발생: " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ws Exception 발생: " << e.what() << std::endl;
    }
    catch (const char *str)
    {
        std::cerr << "ws 에러 발생: " << str << std::endl;
    }
    catch (...)
    {
        std::cerr << "ws 알수 없는 에러 발생" << std::endl;
    }
}