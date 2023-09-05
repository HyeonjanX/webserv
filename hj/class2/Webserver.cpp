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
    std::cout << "디폴트 위치" << "에서 config 읽어옵니다." << std::endl;
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
  // signal(SIGPIPE, SIG_IGN);

  // 2. kqueue 시작
  _eventHandler.eventHandlerInit(); // kqueue() 실패시 throw

  // 3. server 시작 && kqueue에 등록
  initServer(8081);
  initServer(8082);
  initServer(8083);

  // 4. 무한루프에서 돌아가는 코드 시작.
  // runWebserver();
}

void Webserver::initServer(int port, std::string host,
                           int sockreuse, int backlog)
{
  Server s(port, host, sockreuse, backlog);

  _servers.erase(s.getSocket());
  _servers.insert(std::make_pair(s.getSocket(), s));
  _eventHandler.registerReadEvent(s.getSocket());
  std::cout << port << "서버생성" << std::endl;
}

void Webserver::initClient(int serverSocket)
{
  Client c(serverSocket);

  _clients.erase(c.getSocket());
  _clients.insert(std::make_pair(c.getSocket(), c));
  _eventHandler.registerReadWriteEvents(c.getSocket());
  std::cout << "init: " << c.getSocket() << std::endl;
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
        int sock = static_cast<int>(curr.ident);
        Util::print_kevent_info(curr);
        // std::cout << "sock: " << sock << std::endl;

        std::map<int, Server>::iterator sit = _servers.find(sock);
        if (sit != _servers.end())
        {
          Server *s = &sit->second;
          initClient(s->getSocket());
          std::cout << "소켓생성" << std::endl;
          continue;
        }

        std::map<int, Client>::iterator cit = _clients.find(sock);
        if (cit == _clients.end())
        {
          // 알맞은 소켓을 찾지 못했다(가능한건가?) => 에러 핸들링 필요
        }
        std::cout << "============ 클라이언트 ============" << std::endl;

        Client *c = &(cit->second);
        if (curr.filter == EVFILT_READ)
        {
          std::cout << "============** READ **============" << std::endl;
          // 0: 클라이언트 종료 => nothing
          // >0: data += 읽은것.
          // <0: 에러 핸들링
          char buf[10000];
          std::string d;
          ssize_t b;
          b = read(c->getSocket(), buf, 10000);
          // while ((b = read(c->getSocket(), buf, 10000)) > 0)
          // {
          //   d.append(buf, b);
          //   std::cout << "read loop" << std::endl;
          // }
          if (b > 0 )
          {
            d.append(buf, b);
            std::cout << "read: " << d << std::endl;

            c->makeResponse("./lorem_ipsum.txt");

            _eventHandler.switchToWriteState(c->getSocket());

          }
          else if (b == 0)
          {
            std::cout << "클라이언트 연결 종료" << std::endl;
            // close(c->getSocket());
            // _clients.erase(c->getSocket());
            continue;
            // throw "끝";
          }
          else
          {
            std::cout << "????" << std::endl;
          }
          // c->readProcess();
          // if (true) // 다 읽었다면
          // {
          //   c->makeResponse();
          // }
        }
        else if (curr.filter == EVFILT_WRITE)
        {
          std::cout << "============** WRITE **============" << std::endl;
          int result = c->sendProcess();
          (void)result;
          if (c->checkSendBytes() >= 0)
          {
            // 전송 완료
            std::cout << "전송 완료1" << std::endl;
            // _eventHandler.switchToReadState(c->getSocket());
            _eventHandler.switchToReadState(c->getSocket());
            c->cleanRequestReponse();
            std::cout << "전송 완료2" << std::endl;
          }
          // <0: 에러핸들링?
        }
        else
        {
          // 도달 하지 않을 곳.
        }
      }
    }
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception 발생: " << e.what() << std::endl;
  }
}

void Webserver::clientReadProcess(Client &c)
{
  char buffer[1024];
  ssize_t bytes_read = read(c.getSocket(), buffer, 1024);

  if (bytes_read == 0)
  {
    // 클라이언트와의 정상 종료? => 재사용을 위해  => Request와 Response 초기화필요할듯
    std::cout << "클라이언트의 연결이 종료되었습니다." << std::endl;
  }

  else if (bytes_read == -1)
  {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      // 타임아웃 초과
      std::cerr << "Timeout occurred." << std::endl;
    }
    else
    {
      // read() 함수 에러
      std::cerr << "Read error: " << strerror(errno) << std::endl;
    }
    // 1. read 재시도 2. 클라이언트 연결 종료 => 단, 종료전 500류의 에러를 보낼것인지 판단.
  }

  else
  {
    // 읽기 성공 => 처리

    c._data.append(buffer, bytes_read);

    std::cout << "------------------버퍼 데이터------------------" << std::endl;
    std::cout << buffer << std::endl;
    std::cout << "--------------------------------------------" << std::endl;

    if (c._header.empty())
    {
      // 1. 헤더 끝까지 읽기
      size_t pos = c._data.find("\r\n\r\n");
      if (pos == std::string::npos)
      {
        c._header.append(buffer, bytes_read);
      }
      else
      {

        // 헤더를 읽었으니, requestLine의 유효성 판별이나
        // Content-Length들을 파악해 본문을 얼마나 읽으면 되는지 파악

        // c._header = c._data.substr(0, pos);
        // _body = c._data.substr(pos + 4);

        c._header.append(buffer, pos + 2); // 헤더가 \r\n으로 구분가능 하도록!
        c._body.append(buffer + pos + 4, bytes_read - pos - 4);

        std::cout << "클라이언트 입력에서 헤더를 찾았습니다." << std::endl;
      }

      if (c._header.size() > c._headaerLimit)
      {
        // 헤더 길이 초과 체크 => 응답 전송후 클라이언트 연결 종료
      }
    }
    else
    {
      c._body.append(buffer, bytes_read);
    }

    if (!c._header.empty())
    {
      // 2. 본문 끝까지 읽기

      // 2.1 본문 길이 초과 체크
      if (c._body.size() >= c._bodyLimit)
      {
        // 바디 길이 초과 체크 => 응답 전송후 클라이언트 연결 종료
      }

      if (c._ischunk)
      {
        // 2.1 chuncked 처리
        size_t pos = c._body.find("\r\n\r\n");
        if ((pos = c._body.find("\r\n\r\n")) != std::string::npos)
        {
          // 끝을 찾음
          if (pos + 4 == c._body.size())
          {
            // \r\n\r\n으로 끝나야 유효한 요청.
            // \r\n단위로 짤라서 뭔가 하기...
          }
          else
          {
            // 유효하지 않은 요청 => 응답 전송후 클라이언트 연결 종료
          }
        }
      }

      else
      {
        // 2.2 일반적인 read 상태에서 처리
        if (c._body.size() == c._contentLength)
        {
          // 정상 수신
          // 1. 응답 생성 준비
          // 2. 모니터링 WRITE 모드로 전환
        }
        else if (c._body.size() >= c._contentLength)
        {
          // 초과 수신 => 응답 전송후 클라이언트 연결 종료
        }
        else
        {
          // 계속해서 수신
        }
      }
    }
  }

  // return bytes_read;
}
