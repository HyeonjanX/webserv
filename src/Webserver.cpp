#include "Webserver.hpp"

#define DEBUG_PRINT false

// #include "Color.hpp"
// #include "Logger.hpp"
// #include "Session.hpp"

/**
 * 전체 동작 수도 코드로 끄적이기.
 *
 * 1. config파일 읽어서 파싱하기
 * 2. server 시작 (kqueue)
 * 3. kqueue에 server의 소켓 READ 이벤트 모니터링 추가
 * 4.
 *
 * --------- 이하 무한루프에서 실행 되야 할 것들 -------
 *
 * 4. kevent 호출을 통해
 */
Webserver::Webserver(int ac, const char **av)
{

    if (ac > 2 || ac < 1)
    {
        throw("Usage: ./webserv [config_file]");
    }
    else if (ac == 2)
    {
        _configPath = std::string(av[1]);
        std::cout << av[1] << "에서 config를 읽어옵니다." << std::endl;
    }
    else
    {
        _configPath = "./data/json/default.json";
        std::cout << "디폴트 위치" << _configPath << "에서 config 읽어옵니다." << std::endl;
    }

    // 파싱 하여 serverConfigs를 완성

    JsonParser  jsonParser;
    Config      config;

    jsonParser.parseJson(_configPath);
    config.setJson(jsonParser.getJson());

    _serverConfigs = config.makeServerConfigs();
}

Webserver::~Webserver(void)
{
    // 뭔가 해야할까?
}

void Webserver::initWebserver(void)
{
    // Q. 만약 SIGPIPE가 발생하면, 시그널 핸들러로 다시 웹서버를 실행시켜야 할까?
    // 1. 시그널 핸들링
    signal(SIGPIPE, SIG_IGN);

    // 2. kqueue 시작
    _eventHandler.eventHandlerInit(); // kqueue() 실패시 throw

    // 3. server 시작 && kqueue에 등록

    for (std::map<int, std::vector<t_host> >::const_iterator it = _serverConfigs.begin();
        it != _serverConfigs.end(); ++it)
    {
        initServer(it->first, it->second);
    }

    // 4. 랜덤생성 함수를 위해
    srand(time(0));
}

void Webserver::initServer(int port, const std::vector<t_host> &serverConfig)
{
    Server s(port, serverConfig, SOCKET_REUSE_MODE, BACKLOG_VALUE);

    _servers.insert(std::make_pair(s.getSocket(), s));
    _eventHandler.registerReadEvent(s.getSocket());

    std::cout << s << std::endl;
}

void Webserver::initClient(int serverSocket, Server *s)
{
    Client c(serverSocket, this, s, &_eventHandler);

    _clients.erase(c.getSocket());
    _clients.insert(std::make_pair(c.getSocket(), c));
    _eventHandler.registerReadWriteEvents(c.getSocket());
    _eventHandler.registerTimerEvent(c.getSocket(), TIMER_TIME_OUT_SEC);
}

void Webserver::closeClient(Client &c)
{
    int fd;

    if ((fd = c.getCgi().getInPipe(WRITE_FD)) != -1)
        insertToCloseFds(fd);
    if ((fd = c.getCgi().getOutPipe(READ_FD)) != -1)
        insertToCloseFds(fd);
    insertToCloseFds(c.getSocket());
    c.cleanForClose();
    close(c.getSocket());
    _clients.erase(c.getSocket());
}

std::map<int, Client>::iterator Webserver::searchClientByPipeFd(int fd)
{
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second.isPipe(fd))
        {
            return it;
        }
    }
    return _clients.end();
}

std::map<std::string, t_session> &Webserver::getSessions(void) { return _sessions; };

void Webserver::insertToCloseFds(int fd)
{
    if(fd != -1)
        _closeFds.insert(fd);
}

void Webserver::runWebserver(void)
{
    std::map<int, Server>::iterator sit;
    std::map<int, Client>::iterator cit;

    while (1)
    {
        _closeFds.clear();

        int nevents = _eventHandler.newEvents();

        if (nevents == -1)
        {
            std::cerr << "kevent 호출과정에서 -1 리턴" << std::endl;
            throw WebserverException("kevent 호출 실패");
        }

        for (int i = 0; i < nevents; ++i)
        {
            const struct kevent &curr = _eventHandler.getEvent(i);

            if (curr.filter == EVFILT_TIMER && (cit = _clients.find(static_cast<int>(curr.ident))) != _clients.end())
            {
                if (DEBUG_PRINT) std::cout << YELLOW << "***************** ### Client TIMER 이벤트 ### *****************" << RESET << std::endl;
                closeClient(cit->second);
            }
        }

        for (int i = 0; i < nevents; ++i)
        {
            const struct kevent &curr = _eventHandler.getEvent(i);

            if (curr.filter == EVFILT_TIMER && _closeFds.find(curr.ident) != _closeFds.end() && (cit = searchClientByPipeFd(curr.ident)) != _clients.end())
            {
                if (DEBUG_PRINT) std::cout << YELLOW << "***************** ### CGI TIMER 이벤트 ### *****************" << RESET << std::endl;
                cit->second.makeCgiErrorResponse(504); // Gateway Timeout
            }
        }

        for (int i = 0; i < nevents; ++i)
        {
            const struct kevent &curr = _eventHandler.getEvent(i);

            if (_closeFds.find(curr.ident) != _closeFds.end())
            {
                if (DEBUG_PRINT) std::cout << "skip: " << curr.ident << std::endl;
                // 먼저 처리된 이벤트에 의해 삭제된 fd에 대한 이벤트는 건너띄기
            }
            else if ((sit = _servers.find(static_cast<int>(curr.ident))) != _servers.end())
            {
                if (DEBUG_PRINT) std::cout << GREEN << "-------------- 1. 서버 소켓의 READ 이벤트 -----------------" << RESET << std::endl;
                initClient(sit->second.getSocket(), &(sit->second));
            }
            else if ((cit = _clients.find(static_cast<int>(curr.ident))) != _clients.end())
            {
                if (curr.filter == EVFILT_READ)
                {
                    if (curr.flags & EV_EOF)
                    {
                        if (DEBUG_PRINT) std::cout << MAGENTA << "***************** 2. 클라이언트의 소켓 연결 종료 이벤트 *****************" << RESET << std::endl;
                        closeClient(cit->second);
                    }
                    else
                    {
                        if (DEBUG_PRINT) std::cout << CYAN << "***************** 3. 클라이언트 소켓의 READ 이벤트 *****************" << RESET << std::endl;
                        cit->second.readProcess();
                    }
                }
                else if (curr.filter == EVFILT_WRITE)
                {
                    if (DEBUG_PRINT) std::cout << BLUE << "***************** 4. 클라이언트 소켓의 WRITE 이벤트 *****************" << RESET << std::endl;
                    cit->second.sendProcess();
                }
                else
                {
                    // 도달하지 않을 곳
                }
            }                
            else if ((cit = searchClientByPipeFd(curr.ident)) != _clients.end())
            {
                Client *c = &(cit->second);
                Cgi *cgi = &(c->getCgi());

                try
                {
                    if (curr.filter == EVFILT_WRITE)
                    {
                        if (DEBUG_PRINT) std::cout << YELLOW << "***************** 5. CGI WRITE *****************" << RESET << std::endl;
                        cgi->writePipe();
                    }
                    else if (curr.filter == EVFILT_READ)
                    {
                        if (DEBUG_PRINT) std::cout << YELLOW << "***************** 6. CGI READ *****************" << RESET << std::endl;
                        cgi->readPipe();
                        if (curr.flags & EV_EOF)
                            c->makeCgiResponse();
                    }
                    else
                    {
                        // 도달하지 않을 곳
                    }
                }
                catch (int errStatusCode)
                {
                    c->makeCgiErrorResponse(errStatusCode);
                }
            }
            else
            {
                // 도달하지 않을 곳
            }
        }
    }
}
