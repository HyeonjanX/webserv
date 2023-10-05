#include "Cgi.hpp"
#include "Util.hpp"

Cgi::Cgi() : _pid(0), _status(0), _sendBytes(0)
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

        _inPipe[READ_FD] = other._inPipe[READ_FD];
        _inPipe[WRITE_FD] = other._inPipe[WRITE_FD];
        _outPipe[READ_FD] = other._outPipe[READ_FD];
        _outPipe[WRITE_FD] = other._outPipe[WRITE_FD];

        _pid = other._pid;
        _status = other._status;

        _env = other._env;

        _readData = other._readData;
        _postData = other._postData;
        _sendBytes = other._sendBytes;
    }
    return *this;
}

Cgi::~Cgi() {}

int                 &Cgi::getInPipe(int idx) { return _inPipe[idx]; }
int                 &Cgi::getOutPipe(int idx) { return _outPipe[idx]; }
const std::string   &Cgi::getReadData() { return _readData; };
bool                Cgi::allSend() { return _sendBytes >= _postData.size(); }

const std::string   &Cgi::getPostData() const { return _postData; }
void                Cgi::setPostData(std::string postData) { _postData = postData; }

void Cgi::clearCgi()
{
    clearChild();
    clearPipe();
    _pid = 0;
    _status = 0;
    _env.clear();

    _readData.clear();
    _postData.clear();
    _sendBytes = 0;
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

/**
 * @brief
 *
 * @param method
 * @param data
 *
 * @throws const char *msg 어떤 상황에서 오류가 난 것인지 포함
 */
void Cgi::exec(const std::string &method, const std::string &filepath)
{   
    // 0. _postData
    // 1. filepath
    // 2. 권한체크(filepath)

    // 3. pipe 생성 && 논블록 처리
    // if ((pipe(_outPipe) == -1 || fcntl(_outPipe[READ_FD], F_SETFL, O_NONBLOCK, O_CLOEXEC) == -1) ||
    //     (method.compare("POST") == 0 && (pipe(_inPipe) == -1 || fcntl(_inPipe[WRITE_FD], F_SETFL, O_NONBLOCK, O_CLOEXEC) == -1)))
    // {
    //     throw "exec과정에서 pipe() or fcntl() 실패";
    // }

    // POST 아니여도 입력 파이프 생성 버전
    if ((pipe(_outPipe) == -1 || fcntl(_outPipe[READ_FD], F_SETFL, O_NONBLOCK, O_CLOEXEC) == -1) ||
        (pipe(_inPipe) == -1 || fcntl(_inPipe[WRITE_FD], F_SETFL, O_NONBLOCK, O_CLOEXEC) == -1))
    {
        throw "exec과정에서 pipe() or fcntl() 실패";
    }
    
    ;
    if ((_pid = fork()) == -1)
    {
        throw "exec과정에서 fork() 실패";
    }
    else if (_pid == 0)
    {
        // 자녀 프로세스
        // if (dup2(_outPipe[WRITE_FD], STDOUT_FILENO) == -1 ||
        //     (method.compare("POST") == 0 && dup2(_inPipe[READ_FD], STDIN_FILENO) == -1))
        // {
        //     throw "자녀프로세스에서 dup2() 실패";
        // }
        // POST 아니여도 입력 파이프 생성 버전
        if (dup2(_outPipe[WRITE_FD], STDOUT_FILENO) == -1 ||
            dup2(_inPipe[READ_FD], STDIN_FILENO) == -1)
        {
            throw "자녀프로세스에서 dup2() 실패";
        }

        closePipe(_inPipe[READ_FD]);
        closePipe(_inPipe[WRITE_FD]);
        closePipe(_outPipe[READ_FD]);
        closePipe(_outPipe[WRITE_FD]);
        
        execve(filepath.c_str(), NULL, NULL);
        
        throw ExecveException();
    }

    // 4. 부모 => 안 쓰는 파이프 닫기
    closePipe(_inPipe[READ_FD]);
    closePipe(_outPipe[WRITE_FD]);

    // POST 아니여도 입력 파이프 생성 버전
    if (method.compare("POST") != 0 || _postData.empty())
    {
        closePipe(_inPipe[WRITE_FD]);
    }

    // pipePrint();
}

void Cgi::writePipe()
{
    // pipePrint();

    ssize_t bytes = write(_inPipe[WRITE_FD], _postData.data() + _sendBytes, _postData.size() - _sendBytes);
    if (bytes == -1)
    {
        std::cerr << "Fail to write(): " << strerror(errno) << std::endl;
        throw "writePipe()에서 write() 호출 실패"; // 500 응답 생성으로
    }
    _sendBytes += bytes;

    // std::cout << "send: " << bytes << ", total: (" << _sendBytes << "/" << _postData.size() << ")" << std::endl;
    // std::cout << "=============data============" << std::endl;
    // std::cout << _postData.substr(0, bytes) << std::endl;
    // std::cout << "=====....................=====" << std::endl;
}

void Cgi::readPipe()
{
    static std::vector<char> buffer(READ_BUFFER_SIZE);

    ssize_t readBytes = read(_outPipe[READ_FD], buffer.data(), READ_BUFFER_SIZE); // recv, send는 소켓에서만

    if (readBytes <= 0)
    {
        std::cerr << "Fail to read(). fd: " << _outPipe[READ_FD] << ", " << std::string(strerror(errno)) << std::endl;
        throw "readPipe 중 read() 호출 실패"; // 500 응답 생성으로
    }

    _readData.append(buffer.data(), readBytes);
}

// 탐색시 getInPipe[WRITE_FD], getOutPipe[READ_FD]만 필요하다.
bool    Cgi::isPipe(int fd)
{
    return fd != -1 && ( _inPipe[WRITE_FD] == fd || _outPipe[READ_FD] == fd);
}

void    Cgi::pipePrint() const
{
    std::cout << RED << "pipe check: "
        << _inPipe[READ_FD] << ", "
        << _inPipe[WRITE_FD] << ", "
        << _outPipe[READ_FD] << ", "
        << _outPipe[WRITE_FD] << RESET << std::endl;
}

const char *Cgi::ExecveException::what() const throw()
{
    return "CGI 프로그램 execve() 실패";
}