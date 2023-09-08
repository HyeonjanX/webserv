#include <iostream>

#include "Util.hpp"
#include "Webserver.hpp"

int main(int ac, char **av)
{
    // Util::initHttpErrorMap();

    try
    {
        Webserver ws(ac, av);

        ws.initWebserver();
        ws.runWebserver();
    }
    catch (const std::exception &e)
    {
        std::cerr << "ws Exception 발생: " << e.what() << std::endl;
    }
    catch (const char *str)
    {
        std::cerr << "ws 에러 발생: " << std::string(str) << std::endl;
    }
    catch (...)
    {
        std::cerr << "ws 알수 없는 에러 발생" << std::endl;
    }
}