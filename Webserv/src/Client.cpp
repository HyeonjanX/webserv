#include "Client.hpp"

Client::Client(int serverSocket, Webserver *ws, Server *s, EventHandler *e)
    : _ws(ws), _server(s), _eventHandler(e), _contentLength(0), _status(0), _ischunk(0), _erron(0), _defaultBodyNeed(0)
{
    std::memset(&_addr, 0, sizeof(_addr));
    socklen_t addr_size = static_cast<socklen_t>(sizeof(_addr));
    if ((_socket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&_addr), &addr_size)) == -1)
    {
        throw "Fail to accept()" + std::string(strerror(errno));
    }
    // PDF에 FD_CLOEXEC 까지 쓰라는거 아닌가?
    if (fcntl(_socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC) == -1)
    {
        throw "fcntl() error" + std::string(strerror(errno));
    }
    std::cout << "Client 생성: " << _socket << std::endl;
    (void)_server;
    (void)_eventHandler;
}

Client::~Client(void) {}

int Client::getSocket(void) const { return _socket; }

std::string &Client::getData(void) { return _data; }

void Client::readProcess(void)
{
    // 1. 데이터 읽기 => 청크 모드 ? 특별 처리 : 일반처리

    static std::vector<char> buffer(READ_BUFFER_SIZE);
    ssize_t bytes_read = recv(_socket, buffer.data(), READ_BUFFER_SIZE, 0);

    if (bytes_read <= 0)
    {
        std::cerr << "Fail to recv(): " + std::string(strerror(errno)) << std::endl;
        _ws->closeClient(_socket);
        return;
    }

    _request.appendRawData(buffer, bytes_read);

    std::cout << "=========== readProcess =============" << std::endl;
    // std::cout << RED << std::string(buffer.data(), bytes_read) << RESET << std::endl;
    // std::cout << "*************************************" << std::endl;
    std::cout << BLUE << _request.getRawData() << RESET << std::endl;
    std::cout << "-------------------------------------" << std::endl;

    try
    {
        if (_status == BEFORE_READ)
            readRequestLine(); // throw 400 505, to READ_REQUESTLINE
        if (_status == READ_REQUESTLINE)
        {
            readHeader(); // throw 400, to READ_HEADER
            if (_status == READ_HEADER)
            {
                handleHeaders(); // throw 405 30x 405 417, POST && 100 응답 처리 포함
                if (_status == READ_POST_EXPECT_100)
                {
                    std::cout << MAGENTA << "100 응답 생성" << std::endl;
                    _response.generateResponseData();
                    _eventHandler->switchToWriteState(_socket);
                    return;
                }
            }
        }
        if (_status == READ_HEADER)
        {
            // std::cout << MAGENTA << "============= READ_HEADER ===========" << std::endl;
            const std::string &mode = _request.getTransferEncoding();
            if (mode.compare("chunked") == 0)
            {
                chunkRead(); // throw 400 413, to READ_BODY
            }
            else
            {
                readBody(); // throw 400 413, to READ_BODY
            }
        }
        if (_status == READ_BODY)
        {
            // 이곳에 위치할 계획이던 multipart/form-data
            // afterRead()의 POST 호출 직전으로 위치 이동
            _status = READ_END;
        }
        if (_status != READ_END)
            return;
        // 이 아래로는 응답을 생성하고, write모드로 전환이 이뤄진다.
        afterRead();
    }
    catch (int statusCode)
    {
        // 에러 => statusCode를 들고 기본 응답 생성.
        _status = READ_END;
        _response.setStatusCode(statusCode);

        _defaultBodyNeed = 1;
        _erron = 1;
    }

    if (_defaultBodyNeed)
        _response.setBody(createDefaultPage(_response.getStatusCode()));

    makeResponseData();
    _eventHandler->switchToWriteState(_socket);

    std::cout << "afterRead() 종료" << std::endl;

    return;
}

/**
 * 1. requestLine까지 읽은 후, 파싱한다.
 * 2. response의 httpVersion을 세팅한다.
 */
void Client::readRequestLine(void)
{
    // BEFORE_READ => READ_REQUESTLINE

    std::cout << BLUE << "readRequestLine()" << std::endl;

    const std::string rawData = _request.getRawData();
    size_t pos = rawData.find("\r\n");
    if (pos != std::string::npos)
    {
        std::cout << BLUE << rawData.substr(0, pos) << RESET << std::endl;

        _request.parseRequestLine(rawData.substr(0, pos));
        _request.setRawData(rawData.substr(pos + 2));

        _response.setHttpVersion(_request.getHttpVersion());

        _status = READ_REQUESTLINE;

        std::cout << YELLOW << _request.getHttpMethod() << _request.getRequestUrl() << _request.getHttpVersion() << RESET << std::endl;
    }
}

// 헤더 크기 제한 어딘가에 적용하기?
void Client::readHeader(void)
{
    // READ_REQUESTLINE => READ_HEADER

    std::cout << YELLOW << "======== readHeader() ========" << RESET << std::endl;

    std::string::size_type pos = 0;
    std::string::size_type oldPos = 0;
    std::string::size_type colPos;

    const std::string rawData = _request.getRawData();

    while ((pos = rawData.find("\r\n", oldPos)) != std::string::npos)
    {
        if (pos == oldPos)
        {
            _request.setRawData(rawData.substr(pos + 2));
            _status = READ_HEADER;
            return;
        }

        std::string line = rawData.substr(oldPos, pos - oldPos);
        // std::cout << BLUE << "line: |" << line << "|" << RESET << std::endl;
        colPos = line.find(':');
        if (colPos == std::string::npos)
        {
            throw 400; // Bad Request
        }
        std::string key = Util::toLowerCase(line.substr(0, colPos));
        std::string val = Util::lrtrim(line.substr(colPos + 1));
        // std::cout << YELLOW << "헤더 " << key << " : " << val << RESET << std::endl;
        _request.appendHeader(key, val);
        oldPos = pos + 2;
    }

    if (oldPos != 0)
    {
        _request.setRawData(rawData.substr(oldPos));
    }
}

void Client::readBody(void)
{
    // READ_HEADER => READ_BODY
    // throw 400, 413 Payload Too Large

    // rawData == 바디임.
    const std::string &rawData = _request.getRawData();

    // 1. 바디 제한 길이 확인
    // if (rawData.size() >= _bodyLimit)
    //     throw 413; // 413 Payload Too Large | Content Too Large

    // 3. 본문 길이 확인
    size_t contentLength = _request.getContentLength();
    std::cout << GREEN << "크기체크 => rawData:" << rawData.size() << ", contentLength: " << contentLength << RESET << std::endl;
    if (rawData.size() > contentLength)
        throw 400; // 400 or 413

    if (rawData.size() == contentLength)
    {
        _status = READ_BODY;
    }
}

void Client::afterRead(void)
{
    const std::string GET_METHOD("GET");
    const std::string POST_METHOD("POST");
    const std::string DELETE_METHOD("DELETE");

    std::cout << RED << "afterRead() - 요청라인3요소: |"
        << _request.getHttpMethod() << " " << _request.getRequestUrl() << "  " << _request.getHttpVersion() << "|"
        << RESET << std::endl;

    // HTTP Request가 완성 되었다. => method && path
    // 1. GET
    const std::string &method = _request.getHttpMethod();

    std::cout << method << std::endl;

    // 2. location과 매칭해야한다.
    // std::string root(".");
    std::string root("../data");

    // 3. CGI vs Not CGI
    bool cgion = false; // 어디서 어떻게 받아와야 할까?
    if (!cgion)
    {
        if (method.compare(GET_METHOD) == 0)
        {
            std::cout << BLUE << "GET_METHOD()" << RESET << std::endl;
            bool autoindex = _matchedLocation->getAutoindex();
            notCgiGetProcess(root, _request.getRequestPath(), autoindex);
        }
        else if (method.compare(POST_METHOD) == 0)
        {
            std::cout << BLUE << "POST_METHOD()" << RESET << std::endl;
            std::string filepath = root + Util::extractBasename(_request.getRequestPath());
            const std::string &body = _request.getPostData();
            std::cout << "body.size(): " << body.size() << std::endl;
            notCgiPostProcess(filepath, body);
        }
        else if (method.compare(DELETE_METHOD) == 0)
        {
            std::cout << BLUE << "DELETE_METHOD()" << RESET << std::endl;
            std::string filepath = root + _request.getRequestPath();
            notCgiDeleteProcess(filepath);
        }
        else
        {
            // 에러
        }
        // 치명적인 에러 => 응답 필요 X, 클라이언트 삭제
        // 평범한 에러 => 에러 응답 생성 필요 O => 아래로 진행
    }
    else
    {
        // CGI 처리
        if (true)
        {
            // 성공시 kqueue 모니터링 등록후 이동
            return;
        }
        // (실패 == 에러) => 에러 응답하면 됨 => 아래로 진행
    }
    return;
}

int Client::notCgiGetProcess(const std::string &root, const std::string &path, bool autoindex)
{
    try
    {
        std::string fileData = File::getFile(root, path, autoindex);
        _response.setBody(fileData);
        _response.setStatusCode(200);
        _erron = 0;
        _defaultBodyNeed = 0; // 바디는 fileData
    }
    catch (int statusCode)
    {
        _response.setStatusCode(statusCode);
        _erron = 1;           // _erron 삭제 해도 될 수도
        _defaultBodyNeed = 1; // 에러는 기본 바디 필요
    }
    return _status;
}

int Client::notCgiPostProcess(const std::string &filepath, const std::string &body)
{
    try
    {
        File::uploadFile(filepath, body);
        _response.setStatusCode(201); // created
        _erron = 0;
        _defaultBodyNeed = 1; // Post는 바디 콘텐츠 X
    }
    catch (int statusCode)
    {
        _response.setStatusCode(statusCode);
        _erron = 1;
        _defaultBodyNeed = 1;
    }
    return _status;
}

int Client::notCgiDeleteProcess(const std::string &filepath)
{
    try
    {
        File::deleteFile(filepath);
        _response.setStatusCode(200);
        _erron = 0;
        _defaultBodyNeed = 1; // Delete는 바디 콘텐츠 X
    }
    catch (int statusCode)
    {
        _response.setStatusCode(statusCode);
        _erron = 1;
        _defaultBodyNeed = 1;
    }
    return _status;
}

/**
 * 리스폰스 세팅
 * 1.1 _response.헤더<Cotent-length && Date>세팅(리스폰스.바디, 현재:Date)
 * 1.2 리다이렉트 => Location 헤더 설정
 * 2. _response.generateResponseData() 호출: Response가 가진것들을 활용해 _data로 만듬.
 */
void Client::makeResponseData(void)
{
    // 1.1 _response.헤더<Cotent-length && Date>세팅(리스폰스.바디, 현재:Date)
    _response.setHeader(std::string("Content-Length"), std::string(Util::ft_itoa(_response.getBody().length())));
    _response.setHeader(std::string("Date"), Util::getDateString());

    // 1.2 300번대 리다이렉트
    if (_response.getStatusCode() / 100 == 3) // _matchedLocation && _matchedLocation->isRedirect()
    {
        _response.setHeader(std::string("Location"),
            std::string("http://") + _request.findHeaderValue(std::string("host")) + _matchedLocation->getRedirectUrl(_request.getRequestUrl()));
    }

    // 2. _response.generateResponseData() 호출: Response가 가진것들을 활용해 _data로 만듬.
    _response.generateResponseData();
}

std::string Client::createDefaultPage(int statusCode)
{
    const std::string defaultPage("");
    std::string html;
    std::string root(".");

    try
    {
        if (!defaultPage.empty())
        {
            // const std::string &filepath = std::string(".") + defaultPage;
            html = File::getFile(root, defaultPage, false);
        }
        else
        {
            html = createDefaultBody(statusCode);
        }
    }
    catch (int statusCode)
    {
        _response.setStatusCode(statusCode);
        html = createDefaultBody(statusCode);
    }

    return html;
}

std::string Client::createDefaultBody(int statusCode)
{
    const std::string SP(" ");
    std::string body;

    std::string statusCodeComment =
        Util::ft_itoa(statusCode) + SP + Util::getStatusCodeMessage(statusCode);

    body += "<html>";
    body += "<head><title>" + statusCodeComment + "</title></head>";
    body += "<body>";
    body += "<center><h1>" + statusCodeComment + "</h1></center>";
    body += "<hr><center>webserv/hyeonjanX</center>";
    body += "</body>";
    body += "</html>";

    return body;
}

// 현재 리턴값을 활용하고 있진 않음
// EVFILT_WRITE 이벤트에 의해 트리거 되는 곳.
// send 호출후, 모두 보냈는지 확인할 수 있는 값을 리턴,
// send 실패시 throw 가능
void Client::sendProcess(void)
{
    // std::cout << RED << "0: " << _response.getDataLength() << RESET << std::endl;
    std::cout << GREEN << "------- sendProcess -------" << RESET << std::endl;

    ssize_t bytes_sent = send(_socket, _response.getData().c_str(), _response.getDataLength(), 0);

    if (bytes_sent == -1)
    {
        // MSG_NOSIGNAL가 없어서 signal(SIGPIPE, SIG_IGN);를 통해서 SIGPIPE가 들어와도 프로세스가 종료되지 않도록.
        // 클라이언트에서 먼저 종료시: Broken pipe
        std::cerr << "Fail to send(): " + std::string(strerror(errno)) << std::endl;
        _ws->closeClient(_socket);
        return;
    }

    // std::cout << "====================== 보낸 데이터 (응답 확인용) ===============" << std::endl;
    // std::cout << _response.getData().substr(0, bytes_sent) << std::endl;
    // std::cout << "====================== ********* ===============" << std::endl;

    _response.updateData(bytes_sent);
    _response.updateSendedBytes(bytes_sent);

    std::cout << "send: " << bytes_sent << " bytes, (" << _response.getSendBytes() << "/" << _response.getTotalBytes() << ")" << std::endl;

    if (checkSendBytes() >= 0)
    {
        _eventHandler->switchToReadState(_socket);
        if (_status == READ_POST_EXPECT_100)
        {
            std::cout << RED << "100 응답 전송 후" << RESET << std::endl;
            _status = READ_HEADER;
            _response.clean();
            _response.setHttpVersion(_request.getHttpVersion());
            return;
        }
        cleanRequestReponse();
    }

    return;
}

/** return (보낸바이트 - 보내야할바이트), 0 >= 0 이면 송신 끝을 의미 **/
int Client::checkSendBytes() const { return _response.getSendedBytes() - _response.getTotalBytes(); }

void Client::cleanRequestReponse(void)
{
    // 2번째 요청시 문제 발생. 양 clean 코드류에서 문제 발생하는듯.
    _status = BEFORE_READ;
    _ischunk = 0;
    _erron = 0;
    _defaultBodyNeed = 0;
    _request.resetRequest();
    _response.clean();
}

void Client::handleHeaders(void)
{
    const std::string POST_METHOD("POST");

    std::string hostname;

    int statusCode = _request.handleHeaders(hostname);

    std::string root("../data"); // 임시

    if (hostname.empty())
    {
        throw 400;
    }

    // 헤더 => HOST에서 host 매칭 && path에서 location 매칭
    _matchedHost = _server->matchHost(hostname);
    _matchedLocation = _matchedHost->matchLocation(_request.getRequestPath());

    // location->허용메서드 확인
    if (!_matchedLocation->isAllowedMethod(_request.getHttpMethod()))
    {
        throw 405; // Method Not Allowed
    }

    // 리다이렉트 확인
    if (_matchedLocation->isRedirect())
    {
        // TODO: 리다이렉트 상태코드는 30x이도록 체크.
        throw _matchedLocation->getRedirect().first; // 정상적이면 30x에러
    }

    if (statusCode == 100 && _request.getHttpMethod().compare(POST_METHOD) == 0)
    {
        std::string filepath = root + Util::extractBasename(_request.getRequestPath());
        if (File::canUploadFile(filepath)) // 0: o.k
        {
            throw 417; // Expectation Failed
        }
        _status = READ_POST_EXPECT_100;
        _response.setStatusCode(statusCode);
    }
}

/*
chunked-body   = *chunk
                last-chunk
                trailer-part
                CRLF

chunk          = chunk-size [ chunk-ext ] CRLF
                 chunk-data CRLF
chunk-size     = 1*HEXDIG
last-chunk     = 1*("0") [ chunk-ext ] CRLF

chunk-data     = 1*OCTET ; a sequence of chunk-size octets

chunk-ext      = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
chunk-ext-name = token
chunk-ext-val  = token / quoted-string

trailer-part   = *( header-field CRLF )
*/
void Client::chunkRead(void)
{
    const size_t CRLF_SIZE = 2;
    std::string data, chunkOctet;
    std::size_t size, octecPos;

    try
    {
        while (!Util::isLastChunk(_request.getRawData()))
        {
            if ((size = Util::tryReadChunk(_request.getRawData(), octecPos)) == 0)
                return; // 추가 read가 필요함

            _request.getChunkOctetData().append(_request.getRawData().substr(octecPos, size));
            _request.setRawData(_request.getRawData().substr(octecPos + size + CRLF_SIZE));

            // std::cout << RED << "_chunkOctetData.length: " << _request.getChunkOctetData().length() << RESET << std::endl;

            if (_request.getChunkOctetData().size() > _bodyLimit)
            {
                throw 413;
            }
        }

        // LastChunk를 찾아 끝내도 됨.

        std::cout << BLUE << "lastchunk 발견: " << _request.getRawData() << RESET << std::endl;
        std::cout << BLUE << "_chunkOctetData.length: " << _request.getChunkOctetData().length() << RESET << std::endl;

        _status = READ_BODY;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << e.what() << std::endl;
        throw 400;
    }
}