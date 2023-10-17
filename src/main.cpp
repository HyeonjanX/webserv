#include <iostream>

#include "Util.hpp"
#include "Webserver.hpp"

#ifdef DEBUG
#include <cstdlib>
void	check_leaks(void)
{
	system("leaks --list -- webserv");
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
    catch (Webserver::WebserverException &e)
    {
        std::cerr << "Webserver가 종료되게 만드는 오류 발생: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (Cgi::ExecveException &e)
    {
        std::cerr << "자녀프로세스 에러: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception 발생: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const char *str)
    {
        std::cerr << "const char *str 에러 발생: " << str << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "알 수 없는 에러 발생" << std::endl;
        std::cerr << "errno(" << errno << "): " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
