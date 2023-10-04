#include "Cgi.hpp"


Cgi::Cgi() : _pid(0), _status(0)
{
    _inPipe[READ_FD] = -1;
    _inPipe[WRITE_FD] = -1;
    _outPipe[READ_FD] = -1;
    _outPipe[WRITE_FD] = -1;
}

Cgi::Cgi(const Cgi &other) { *this = other; }

Cgi &Cgi::operator=(const Cgi &other)
{
    if (this != &other)
    {
        // _request = other._request;
        // _response = other._response;
        // _client = other._client;
        
        _inPipe[READ_FD] = other._inPipe[READ_FD];
        _inPipe[WRITE_FD] = other._inPipe[WRITE_FD];
        _outPipe[READ_FD] = other._outPipe[READ_FD];
        _outPipe[WRITE_FD] = other._outPipe[WRITE_FD];
        
        _pid = other._pid;
        _status = other._status;
        
        _env = other._env;
    }
    return *this;
}

Cgi::~Cgi() {}

void Cgi::clearCgi()
{
    clearChild();
    clearPipe();
    _pid = 0;
    _status = 0;
    _env.clear();
}

void Cgi::clearChild()
{
    if (_pid)
    {
        kill(_pid, SIGKILL);
        _pid = 0;
    }
}

void Cgi::clearPipe()
{
    closePipe(_inPipe[READ_FD]);
    closePipe(_inPipe[WRITE_FD]);
    closePipe(_inPipe[READ_FD]);
    closePipe(_inPipe[WRITE_FD]);
}

void Cgi::closePipe(int &fd)
{
    if (fd != -1 && close(fd) == -1)
    {
        std::cerr << "closePipe error: " << strerror(errno) << std::endl;
    }
    fd = -1;
}

void Cgi::exec(const std::string &method, const std::string &data)
{
    // std::string method = "POST";
    // 1. path
    std::string path = "./test.py";

    // 2. 권한체크(path)

    // 3. pipe
    if (pipe(_outPipe) == -1 || 
        (method.compare("POST") == 0 && pipe(_inPipe) == -1))
    {
        throw "exec과정에서 pipe() 실패";
    }
    std::cout << "pipe1 : (" << _inPipe[READ_FD] << ", " << _inPipe[WRITE_FD] << "), (" << _outPipe[READ_FD] << ", " << _outPipe[WRITE_FD] << ")" << std::endl;
    
    _pid = fork();
    if (_pid == -1)
    {
        throw "exec과정에서 fork() 실패";
    }
    else if (_pid == 0)
    {
        // 자녀 프로세스
        if (dup2(_outPipe[WRITE_FD], STDOUT_FILENO) == -1 ||
            (method.compare("POST") == 0 && dup2(_inPipe[READ_FD], STDIN_FILENO) == -1))
        {
            throw "자녀프로세스에서 dup2() 실패";
        }

        closePipe(_inPipe[READ_FD]);
        closePipe(_inPipe[WRITE_FD]);
        closePipe(_outPipe[READ_FD]);
        closePipe(_outPipe[WRITE_FD]);

        char *filename = "/usr/bin/python3";  // Python3 인터프리터의 경로
        char *argv[] = { "python3", "./test.py", NULL };  // Python3와 실행할 파이썬 스크립트
        char *envp[] = { "REQUEST_METHOD=GET", "QUERY_STRING=id=42", NULL };  // 환경 변수 설정

        execve(filename, argv, envp);
        throw "자녀프로세스에서 execve() 실패";
    }

    // 4. 부모 => 안 쓰는 파이프 닫기
    closePipe(_inPipe[READ_FD]);
    closePipe(_outPipe[WRITE_FD]);

    // 5. 논블록 처리;
    // if (fcntl(_outPipe[READ_FD], F_SETFL, O_NONBLOCK) == -1 || 
    //     (method.compare("POST") == 0 && fcntl(_inPipe[WRITE_FD], F_SETFL, O_NONBLOCK) == -1))
    // {
    //     throw "exec과정에서 pipe() 실패";
    // }

    // 6. 이벤트 등록
    if (method.compare("POST") == 0)
    {
        // POST
        // _eventHandler.addKeventToChangeList(
            _inPipe[WRITE_FD], EVFILT_WRITE, EV_ADD, 0, 0, static_cast<void*>(this));
        // _eventHandler.addKeventToChangeList(
            _outPipe[READ_FD], EVFILT_READ, EV_ADD | EV_DISABLE, 0, 0, static_cast<void*>(this));

        write(_inPipe[WRITE_FD], data.c_str(), data.size());
        closePipe(_inPipe[WRITE_FD]);
    }
    else
    {
        // GET
        // _eventHandler.addKeventToChangeList(_outPipe[READ_FD], EVFILT_READ, EV_ADD, 0, 0, static_cast<void*>(this));
    }

    std::cout << "pipe2 : (" << _inPipe[READ_FD] << ", " << _inPipe[WRITE_FD] << "), (" << _outPipe[READ_FD] << ", " << _outPipe[WRITE_FD] << ")" << std::endl;



    char buffer[10];
    std::string readData;
    ssize_t bytes_read;
    do
    {
        bytes_read = read(_outPipe[READ_FD], buffer, sizeof(buffer));
        readData.append(buffer, bytes_read);
    } while (bytes_read > 0);
    
    if (bytes_read == -1)
    {
        std::cerr << strerror(errno) << std::cout;
    }
    else
    {
        // bytes_read == -0
        std::cout << "================ CGI READ ==============" << std::endl;
        std::cout << "bytes_read :" << bytes_read << std::endl;
        std::cout << readData << std::endl;
        std::cout << "================ * * * * * ==============" << std::endl;
    }

    closePipe(_outPipe[READ_FD]);
    
    clearCgi();
}
