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
    // _eventHandler.registerTimerEvent(c.getSocket(), TIMER_KEEP_ALIVE_SEC);
    _eventHandler.registerTimerEvent(c.getSocket(), TIMER_TIME_OUT_SEC);
    if (DEBUG_PRINT) std::cout << "initClient: fd(" << c.getSocket() << ")" << std::endl;
}

void Webserver::runWebserver(void)
{
    // Q. 이미 바깥에 try 구문이 있는데, 여기서 한 번 더 사용해야 할 필요가 있을까?
    try
    {
        while (1)
        {
            int nevents = _eventHandler.newEvents();
            if (DEBUG_PRINT)
            {
                std::cout << "nevents: " << nevents << std::endl;
                std::cout << "_clients: " << _clients.size() << std::endl;
                std::cout << "_servers: " << _servers.size() << std::endl;
            }

            for (int i = 0; i < nevents; ++i)
            {
                struct kevent curr = _eventHandler.getEvent(i);

                std::map<int, Client>::iterator cit = _clients.find(static_cast<int>(curr.ident));
                if (cit != _clients.end() && curr.filter == EVFILT_TIMER)
                {
                    // Q. 잘못된 주석이죠...??
                    // 4. 클라이언트 소켓의 WRITE 이벤트
                    if (DEBUG_PRINT) std::cout << YELLOW << "***************** Client TIMER *****************" << RESET << std::endl;

                    Client *c = &(cit->second);

                    closeClient(*c);

                    continue;
                }
            }

            for (int i = 0; i < nevents; ++i)
            {
                struct kevent curr = _eventHandler.getEvent(i);

                // 디버깅용 정보 출력
                // Util::print_kevent_info(curr);

                std::map<int, Server>::iterator sit = _servers.find(static_cast<int>(curr.ident));
                if (sit != _servers.end())
                {
                    // 1. 서버소켓의 READ 이벤트
                    if (DEBUG_PRINT) std::cout << GREEN << "-------------- 소켓생성 -----------------" << RESET << std::endl;

                    Server *s = &sit->second;

                    initClient(s->getSocket(), s);

                    continue;
                }

                std::map<int, Client>::iterator cit = _clients.find(static_cast<int>(curr.ident));

                if (cit != _clients.end())
                {
                    // 클라이언트가 FIN요청을 보내서, 연결이 종료됨 => 재사용 불가 => 이미 닫힌 연결 => 안 닫으면 계속해서 READ 이벤트가 발생.
                    if (curr.filter == EVFILT_READ && (curr.flags & EV_EOF))
                    {
                        // 2. 클라이언트의 소켓 연결 종료 이벤트
                        if (DEBUG_PRINT) std::cout << MAGENTA << "***************** 클라이언트부터 소켓 연결 종료 *****************" << RESET << std::endl;

                        Client *c = &(cit->second);

                        closeClient(*c);

                        continue;
                    }

                    if (curr.filter == EVFILT_READ)
                    {
                        // 3. 클라이언트 소켓의 READ 이벤트
                        if (DEBUG_PRINT) std::cout << CYAN << "***************** Client READ *****************" << RESET << std::endl;

                        Client *c = &(cit->second);

                        c->readProcess();

                        continue;
                    }

                    if (curr.filter == EVFILT_WRITE)
                    {
                        // 4. 클라이언트 소켓의 WRITE 이벤트
                        if (DEBUG_PRINT) std::cout << BLUE << "***************** Client WRITE *****************" << RESET << std::endl;

                        Client *c = &(cit->second);

                        c->sendProcess();

                        continue;
                    }
                }                

                std::map<int, Client>::iterator cit2 = searchClientByPipeFd(curr.ident);

                if (cit2 != _clients.end())
                {
                    Client *c = &(cit2->second);
                    Cgi *cgi = &(c->getCgi());

                    try
                    {
                        if (curr.filter == EVFILT_WRITE)
                        {
                            if (DEBUG_PRINT) std::cout << YELLOW << "***************** CGI WRITE *****************" << RESET << std::endl;
                            cgi->writePipe(); // write() failt시 => 500 응답 생성 코스로
                        }
                        else if (curr.filter == EVFILT_READ)
                        {
                            if (DEBUG_PRINT) std::cout << YELLOW << "***************** CGI READ *****************" << RESET << std::endl;
                            cgi->readPipe(); // read() fail시 => 500 응답 생성 코스로
                            if (curr.flags & EV_EOF)
                                c->makeCgiResponse();
                        }
                    }
                    catch (const char *msg)
                    {
                        std::cerr << "CGI 동작 중 문제 발생: " << msg << std::endl;
                        c->makeCgiErrorResponse();
                    }

                    continue;
                }

                // 도달 하지 않을 곳.
            }
        }
    }
    catch (Cgi::ExecveException &e)
    {
        std::cerr << "자녀프로세스: " << e.what() << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception 발생: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "runWebserver() 실행중 알 수 없는 에러 발생" << std::endl;
        std::cerr << "errno(" << errno << "): " << strerror(errno) << std::endl;
    }
}

void Webserver::closeClient(Client &c)
{
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
