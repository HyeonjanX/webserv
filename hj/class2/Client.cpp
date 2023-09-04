
#include "Client.hpp"

#include <fcntl.h>

Client::Client(int serverSocket)
    : _bytes_read(0), _contentLength(0), _readMode(READ_NORMAL);
{
    std::memset(&_addr, 0, sizeof(_addr));
    if ((_socket = accept(serverSocket, (struct sockaddr *)&_addr, sizeof(_addr)) == -1))
    {
        throw "Fail to accept()" + std::string(strerror(errno));
    }
    // PDF에 FD_CLOEXEC 까지 쓰라는거 아닌가?
    if (fcntl(_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC) == -1)
    {
        throw "fcntl() error" + std::string(strerror(errno));
    }
}

Client::~Client(void)
{
}

ssize_t Client::read(void)
{
    char buffer[1024];
    ssize_t bytes_read = read(_socket, buffer, 1024);

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

        _data.append(buffer, bytes_read);

        std::cout << "------------------버퍼 데이터------------------" << std::endl;
        std::cout << c.buffer << std::endl;
        std::cout << "--------------------------------------------" << std::endl;

        if (_header.empty())
        {
            // 1. 헤더 끝까지 읽기

            if ((size_t pos = _data.find("\r\n\r\n")) == std::string::npos)
            {
                _header.append(buffer, bytes_read);
            }
            else
            {

                // 헤더를 읽었으니, requestLine의 유효성 판별이나
                // Content-Length들을 파악해 본문을 얼마나 읽으면 되는지 파악

                // _header = _data.substr(0, pos);
                // _body = _data.substr(pos + 4);

                _header.append(buffer, pos + 2); // 헤더가 \r\n으로 구분가능 하도록!
                _body.append(buffer + pos + 4, bytes_read - pos - 4);

                std::cout << "클라이언트 입력에서 헤더를 찾았습니다." << std::endl;
            }

            if (_header.size() > _headaerLimit)
            {
                // 헤더 길이 초과 체크 => 응답 전송후 클라이언트 연결 종료
            }
        }
        else
        {
            _body.append(buffer, bytes_read);
        }

        bool read_end = false;

        if (!_header.empty())
        {
            // 2. 본문 끝까지 읽기

            // 2.1 본문 길이 초과 체크
            if (_body.size() >= _bodyLimit)
            {
                // 바디 길이 초과 체크 => 응답 전송후 클라이언트 연결 종료
            }

            if (_readMode == READ_CHUNKED)
            {
                // 2.1 chuncked 처리
                if ((size_t pos = _body.find("\r\n\r\n")) != std::string::npos)
                {
                    // 끝을 찾음
                    if (pos + 4 == _body.size())
                    {
                        // \r\n\r\n으로 끝나야 유효한 요청.
                        read_end = true;
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
                if (_body.size() == _contentLength)
                {
                    // 정상 수신
                    // 1. 응답 생성 준비
                    // 2. 모니터링 WRITE 모드로 전환
                }
                else if (_body.size() >= _contentLength)
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

    return bytes_read;
}

ssize_t Client::send(void)
{
    // read, write가 굳이 여기에 있어야 한다면.......
    // 리턴값은 결과 상태를 나타낼수 있는 무언가가 좋을듯?
}

int getSocket(void) const { return _socket; }