#include <iostream>

#include "Util.hpp"
#include "Webserver.hpp"

#ifdef DEBUG
#include <cstdlib>
void	check_leaks(void)
{
	system("leaks --list -- Webserv");
}
#define ATEXIT_CHECK() atexit(check_leaks)
#else
#define ATEXIT_CHECK()
#endif

void    signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        ATEXIT_CHECK();
    	std::exit(1);
    }
}

int main(int ac, const char **av)
{
    signal(SIGINT, signalHandler);

    try
    {
        Webserver ws(ac, av);

        ws.initWebserver();
        ws.runWebserver();
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "ws runtime_error 발생: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ws Exception 발생: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const char *str)
    {
        std::cerr << "ws 에러 발생: " << str << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "ws 알수 없는 에러 발생" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
