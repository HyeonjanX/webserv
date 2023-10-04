#include "Webserver.hpp"

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
// 아래 이니셜라이저
Webserver::Webserver(int ac, char **av)
{
  // EventHandler _eventHandler; => 명시적 이니셜라이저에서 호출하지 않으면, 기본생성자로 생성 => O.K
  // std::string _config;
  // std::map<Server> _servers;
  // std::map<Server> _clients;

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
    _configPath = "./default/path/to/config";
    std::cout << "디폴트 위치"
              << "에서 config 읽어옵니다." << std::endl;
  }
  // config 파싱 코드 들어갈 자리
}

Webserver::~Webserver(void)
{
  // 뭔가 해야할까?
}

void Webserver::initWebserver(void)
{
  // 1. 시그널 핸들링
  signal(SIGPIPE, SIG_IGN);

  // 2. kqueue 시작
  _eventHandler.eventHandlerInit(); // kqueue() 실패시 throw

  // 3. server 시작 && kqueue에 등록
  initServer(8081);
  initServer(8082);
  initServer(8083);

  // 4. 무한루프에서 돌아가는 코드 시작.
  // runWebserver();
}

void Webserver::initServer(int port, int sockreuse, int backlog)
{
  Server s(port, sockreuse, backlog);

  _servers.insert(std::make_pair(s.getSocket(), s));
  _eventHandler.registerReadEvent(s.getSocket());

  // std::cout << port << "서버생성" << std::endl; // 아래 출력으로 대체
  std::cout << s << std::endl;
}

void Webserver::initClient(int serverSocket, Server *s)
{
  Client c(serverSocket, this, s, &_eventHandler);

  _clients.erase(c.getSocket());
  _clients.insert(std::make_pair(c.getSocket(), c));
  _eventHandler.registerReadWriteEvents(c.getSocket());
  std::cout << "initClient: fd(" << c.getSocket() << ")" << std::endl;
}

void Webserver::runWebserver(void)
{
  try
  {
    while (1)
    {
      int nevents = _eventHandler.newEvents();
      std::cout << "nevents: " << nevents << std::endl;
      std::cout << "_clients: " << _clients.size() << std::endl;
      std::cout << "_servers: " << _servers.size() << std::endl;
      for (int i = 0; i < nevents; ++i)
      {
        struct kevent curr = _eventHandler.getEvent(i);

        // 디버깅용 정보 출력
        // Util::print_kevent_info(curr);

        std::map<int, Server>::iterator sit = _servers.find(static_cast<int>(curr.ident));
        if (sit != _servers.end())
        {
          // 1. 서버소켓의 READ 이벤트
          std::cout << GREEN << "-------------- 소켓생성 -----------------" << RESET << std::endl;

          Server *s = &sit->second;

          initClient(s->getSocket(), s);

          continue;
        }

        std::map<int, Client>::iterator cit = _clients.find(static_cast<int>(curr.ident));

        // 클라이언트가 FIN요청을 보내서, 연결이 종료됨 => 재사용 불가 => 이미 닫힌 연결 => 안 닫으면 계속해서 READ 이벤트가 발생.
        if (cit != _clients.end() && curr.filter == EVFILT_READ && (curr.flags & EV_EOF))
        {
          // 2. 클라이언트의 소켓 연결 종료 이벤트
          std::cout << MAGENTA << "***************** 클라이언트부터 소켓 연결 종료 *****************" << RESET << std::endl;

          Client *c = &(cit->second);

          closeClient(c->getSocket());

          continue;
        }

        if (cit != _clients.end() && curr.filter == EVFILT_READ)
        {
          // 3. 클라이언트 소켓의 READ 이벤트
          std::cout << CYAN << "***************** Client READ *****************" << RESET << std::endl;

          Client *c = &(cit->second);

          c->readProcess(); // recv() fail시 _ws->closeClient(_socket);

          continue;
        }

        if (cit != _clients.end() && EVFILT_WRITE)
        {
          // 4. 클라이언트 소켓의 WRITE 이벤트
          std::cout << BLUE << "***************** Client WRITE *****************" << RESET << std::endl;

          Client *c = &(cit->second);

          c->sendProcess(); // send() fail시 _ws->closeClient(_socket);

          continue;
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
              std::cout << YELLOW << "***************** CGI WRITE *****************" << RESET << std::endl;
              cgi->writePipe(); // write() failt시 => 500 응답 생성 코스로
              if (cgi->allSend())
              {
                cgi->closePipe(cgi->getInPipe(WRITE_FD));
                _eventHandler.addKeventToChangeList(
                  cgi->getOutPipe(READ_FD), EVFILT_READ, EV_ENABLE, 0, 0, static_cast<void *>(&cgi));
              }
            }
            else if (curr.filter == EVFILT_READ)
            {
              std::cout << YELLOW << "***************** CGI READ *****************" << RESET << std::endl;
              if (curr.flags & EV_EOF)
              {
                std::cout << MAGENTA << "EV_EOF" << RESET << std::endl;
                c->makeCgiResponse();
              }
              else
              {
                std::cout << MAGENTA << "*읽기" << RESET << std::endl;
                cgi->readPipe(); // read() fail시 => 500 응답 생성 코스로
              }
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

// kqueue 이벤트는 명시적으로 삭제할 필요 없다.
void Webserver::closeClient(int clientsocket)
{
  close(clientsocket);
  _clients.erase(clientsocket);
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
