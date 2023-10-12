#include "Client.hpp"

#define DEBUG_PRINT false
#define DEBUG_SESSION_PRINT false

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

    // For 시지 테스트
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    if (setsockopt(_socket, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)))
    {
        throw("setsockopt(SO_LINGER) for client failed");
    }

    if (DEBUG_PRINT)
        std::cout << "Client 생성: " << _socket << std::endl;
}

Client::~Client(void) {}

static std::map<std::string, std::string> parseCookies(const std::string &cookieHeader)
{
    std::map<std::string, std::string> cookies;
    std::stringstream ss(cookieHeader);
    std::string token;

    while (std::getline(ss, token, ';'))
    {
        std::string::size_type pos = token.find('=');
        if (pos != std::string::npos)
        {
            std::string key = Util::lrtrim(token.substr(0, pos));
            std::string value = Util::lrtrim(token.substr(pos + 1));
            cookies[key] = value;
        }
    }

    return cookies;
}

static std::string generateSessionId(const std::map<std::string, t_session> &sessions)
{
    const int sessionIdSize = 20;
    std::string sessionId;
    const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; // 사용할 문자들

    do
    {
        sessionId.clear();
        for (int i = 0; i < sessionIdSize; ++i)
            sessionId += alphanum[rand() % (sizeof(alphanum) - 1)];
    } while (sessions.find(sessionId) != sessions.end());

    return sessionId;
}

static void handleCgiHeaders(const std::string &header, int &statusCode)
{
    std::vector<std::pair<std::string, std::string> > keyValuePairs = Util::getKeyValuePairs(header);
    for (std::vector<std::pair<std::string, std::string> >::const_iterator it = keyValuePairs.begin();
         it != keyValuePairs.end(); ++it)
    {
        if (it->first.compare("status") == 0)
        {
            std::string value = it->second;
            std::string code, msg;
            std::size_t spPos = value.find(' ');
            if (spPos != std::string::npos)
            {
                code = value.substr(0, spPos);
                msg = Util::lrtrim(value.substr(spPos + 1));
            }
            else
                code = value;
            char *end;

            long num = std::strtol(code.c_str(), &end, 10);

            if (*end == 0 && 1 <= num && num <= 599)
                statusCode = static_cast<int>(num);
            return;
        }
    }
}

static int canExcuteCgi(const std::string &method, const std::string &cgiExt, const std::string &filepath)
{
    const bool isNeedFileCheck = false;
    std::string programPath;

    if (method.compare("GET") && method.compare("POST"))
        return 405;

    if (isNeedFileCheck && !File::checkFileExist(filepath))
        return 404;

    if (cgiExt.compare(".bla") == 0)
        programPath = "tester/cgi_tester";
    else if (cgiExt.compare(".py") == 0)
        programPath = "/usr/bin/python3";
    else if (cgiExt.compare(".php") == 0)
        programPath = "/usr/bin/php";
    else
        return 501; // Not Implemented

    if (File::canExecuteFile(programPath))
        return 503; // Service Unavailable

    return 0;
}

int Client::getSocket(void) const { return _socket; }
Request &Client::getRequest() { return _request; }
Response &Client::getResponse() { return _response; }
Cgi &Client::getCgi() { return _cgi; }

/**
 * @brief 클라이어트 READ 버퍼 읽기 호출시 트리거
 * - 응답 생성 or 100 응답 or CGI 실행
 */
void Client::readProcess(void)
{
    int statusCode;

    if (receiveRequest() <= 0)
    {
        std::cerr << "Fail to recv(): " + std::string(strerror(errno)) << std::endl;
        _ws->closeClient(*this);
        return;
    }

    try
    {
        parseRequest();
        if (_status == READ_POST_EXPECT_100)
        {
            if (DEBUG_PRINT)
                std::cout << MAGENTA << "100 응답 생성" << std::endl;
            _response.generate100ResponseData();
            _eventHandler->switchToWriteState(_socket);
            return;
        }
        if (_status != READ_END)
            return;
        // 이 아래로는 응답을 생성하고, write모드로 전환이 이뤄진다. (not cgi)
        // cgi는 정상 동작시, kqueue 모니터링 등록한다. (cgi)
        statusCode = doRequest();
        if (_status == EXEC_CGI)
        {
            _eventHandler->turnOffRead(_socket);
            return;
        }
    }
    catch (int errorStatusCode)
    {
        // 30x 리다이렉트 코드도 이리로 온다.
        if (DEBUG_PRINT)
            std::cout << RED << "readProcess catch: " << errorStatusCode << RESET << std::endl;
        _status = READ_END;
        statusCode = errorStatusCode;
        _defaultBodyNeed = 1;
        _erron = 1;
    }

    if (_request.getHttpMethod().compare("HEAD") == 0)
        _defaultBodyNeed = 0;
    makeResponseData(statusCode, _defaultBodyNeed);
    _eventHandler->switchToWriteState(_socket);

    return;
}

ssize_t Client::receiveRequest(void)
{
    // 1. 데이터 읽기 => 청크 모드 ? 특별 처리 : 일반처리

    static std::vector<char> buffer(READ_BUFFER_SIZE);
    ssize_t bytes_read = recv(_socket, buffer.data(), READ_BUFFER_SIZE, 0);

    if (bytes_read > 0)
    {
        _request.appendRawData(buffer, bytes_read);

        if (DEBUG_PRINT)
        {
            std::cout << "=========== readProcess =============" << std::endl;
            // std::cout << RED << std::string(buffer.data(), bytes_read) << RESET << std::endl;
            // std::cout << "*************************************" << std::endl;
            std::cout << BLUE << _request.getRawData() << RESET << std::endl;
            std::cout << "-------------------------------------" << std::endl;
        }
    }

    return bytes_read;
}

void Client::parseRequest(void)
{
    if (DEBUG_PRINT)
        std::cout << "=========== parseRequest() ===========" << std::endl;
    
    if (_status == BEFORE_READ)
    {
        _eventHandler->registerTimerEvent(_socket, TIMER_TIME_OUT_SEC);
        _status = READ_STARTED;
    }
    if (_status == READ_STARTED)
        readRequestLine(); // throw 400 505
    if (_status == READ_REQUESTLINE)
    {
        readHeader(); // throw 400
        if (_status == READ_HEADER)
            handleHeaders(); // throw 405 30x 405 417, to READ_POST_EXPECT_100
    }
    if (_status == READ_HEADER)
        readBody();
    if (_status == READ_BODY)
    {
        if (DEBUG_PRINT)
            std::cout << "=========== READ_BODY ===========" << std::endl;
        // 이곳에 위치할 계획이던 multipart/form-data
        // doRequest()의 POST 호출 직전으로 위치 이동
        _status = READ_END;
    }
}

/**
 * 1. requestLine까지 읽은 후, 파싱한다.
 * 2. response의 httpVersion을 세팅한다.
 */
void Client::readRequestLine(void)
{
    // BEFORE_READ => READ_REQUESTLINE

    if (DEBUG_PRINT)
        std::cout << BLUE << "=========== readRequestLine() ===========" << RESET << std::endl;

    const std::string &rawData = _request.getRawData();

    size_t pos = rawData.find("\r\n");

    if (pos != std::string::npos)
    {
        if (DEBUG_PRINT)
            std::cout << BLUE << rawData.substr(0, pos) << RESET << std::endl;

        _request.parseRequestLine(rawData.substr(0, pos));
        _request.setRawData(rawData.substr(pos + 2));

        _response.setHttpVersion(_request.getHttpVersion());

        _status = READ_REQUESTLINE;

        if (DEBUG_PRINT)
            std::cout << YELLOW << "readRequestLine에서 파싱한 결과 : " << _request.getHttpMethod() << " " << _request.getRequestUrl() << " " << _request.getHttpVersion() << RESET << std::endl;
    }
}

// 헤더 크기 제한 어딘가에 적용하기?
void Client::readHeader(void)
{
    // READ_REQUESTLINE => READ_HEADER

    if (DEBUG_PRINT)
        std::cout << YELLOW << "======== readHeader() ========" << RESET << std::endl;

    std::string::size_type pos = 0, oldPos = 0, colPos;

    const std::string &rawData = _request.getRawData();

    while ((pos = rawData.find(CRLF, oldPos)) != std::string::npos)
    {
        if (pos == oldPos)
        {
            // 헤더 끝까지 읽기 완료
            _request.setRawData(rawData.substr(pos + 2));
            _status = READ_HEADER;
            return;
        }

        std::string line = rawData.substr(oldPos, pos - oldPos);

        if ((colPos = line.find(':')) == std::string::npos)
            throw 400; // Bad Request

        std::string key = Util::toLowerCase(line.substr(0, colPos));
        std::string val = Util::lrtrim(line.substr(colPos + 1));

        _request.appendHeader(key, val);

        oldPos = pos + CRLF_SIZE;
    }

    if (oldPos != 0)
    {
        _request.setRawData(rawData.substr(oldPos));
    }
}

/**
 * @brief 컨테츠 길이만큼 or Chunked 규칙에 맞게 Body를 읽습니다.
 *  - 바디 제한 길이 && ContentLength와 비교
 * @throws int statusCode: 400 잘못된 컨텐츠 길이, 413 Content Too Large
 */
void Client::readBody(void)
{
    if (DEBUG_PRINT)
        std::cout << "=========== readBody() ===========" << std::endl;

    if (_request.getTransferEncoding().compare("chunked") == 0)
        chunkRead(); // throw 400 413
    else
    {
        const size_t readBodyLength = _request.getRawData().size();
        const size_t contentLength = _request.getContentLength();
        const size_t clientMaxBodySize = _matchedLocation->getClientMaxBodySize();

        if (DEBUG_PRINT)
            std::cout << GREEN << "크기체크 => rawData:" << readBodyLength << ", contentLength: " << contentLength << ", clientMaxBodySize: " << clientMaxBodySize << RESET << std::endl;

        // TODO: FIX
        if (readBodyLength >= clientMaxBodySize)
            throw 413; // 413 Content Too Large
        if (readBodyLength > contentLength)
            throw 400;
        if (readBodyLength == contentLength)
            _status = READ_BODY;
    }
}

/**
 * @brief
 *
 * @param method
 * @return int stausCode: Response 응답코드에 사용할 값이다.
 *
 * @throws
 * - 400:
 * - 405: GET / POST / DELETE 외 메서드는 지원 X
 */
int Client::doNonCgiProcess(const std::string &method)
{
    const std::string GET_METHOD("GET");
    const std::string POST_METHOD("POST");
    const std::string DELETE_METHOD("DELETE");

    const std::string &root = _matchedLocation->getRoot();
    const std::string &path = _request.getRequestPath();
    const std::string &uri = _matchedLocation->getUri();

    if (DEBUG_PRINT)
        std::cout << YELLOW << "doNonCgiProcess => root: " << root << " path: " << path << std::endl;

    if (DEBUG_PRINT)
        std::cout << BLUE << method << "_METHOD()" << RESET << std::endl;

    if (method.compare(GET_METHOD) == 0)
    {
        bool autoindex = _matchedLocation->getAutoindex();
        const std::vector<std::string> &index = _matchedLocation->getIndex();

        return notCgiGetProcess(root, path, autoindex, index);
    }
    else if (method.compare(POST_METHOD) == 0)
    {
        // POST의 경우, 경로를 제외한 path만 받아들인다.
        std::string filepath = root + Util::extractBasename(path);

        const std::string &body = _request.getPostData(); // throw 400

        // std::cout << "body.size(): " << body.size() << std::endl;

        return notCgiPostProcess(filepath, body);
    }
    else if (method.compare(DELETE_METHOD) == 0)
    {
        std::string filepath = Util::getRootedPath(path, uri, root);

        return notCgiDeleteProcess(filepath);
    }
    else if (method.compare("PUT") == 0)
    {
        std::string filepath = root + Util::extractBasename(path);
        const std::string &body = _request.getPostData(); // throw 400
        return notCgiPostProcess(filepath, body);
    }
    else if (method.compare("HEAD") == 0)
    {
        int statusCode;
        try
        {
            bool autoindex = _matchedLocation->getAutoindex();
            const std::vector<std::string> &index = _matchedLocation->getIndex();
            const std::string &filepath = Util::getRootedPath(path, uri, root);
            File::getFile(path, filepath, autoindex, index);
            statusCode = 200;
            _erron = 0;
            _defaultBodyNeed = 0; // HEAD 요청은 바디 X
        }
        catch (int errorStatusCode)
        {
            statusCode = errorStatusCode;
            _erron = 1;           // _erron 삭제 해도 될 수도
            _defaultBodyNeed = 0; // HEAD 요청은 바디 X
        }
        return statusCode;
    }
    else
    {
        throw 405; // 405 Method Not Allowed
    }
}

/**
 * @brief
 *
 * @return int statusCode: Response 응답에 쓰일 값이다.
 *
 * @throws
 * - 400:
 * - 405: GET / POST / DELETE 외 메서드는 지원 X
 * - 500
 */
int Client::doRequest(void)
{
    if (DEBUG_PRINT)
    {
        std::cout << RED << "doRequest() - 요청라인3요소: |"
                  << _request.getHttpMethod() << " " << _request.getRequestUrl() << "  " << _request.getHttpVersion() << "|"
                  << RESET << std::endl;
    }

    const std::string &method = _request.getHttpMethod();
    const std::string &path = _request.getRequestPath();
    const std::string &cgiExt = _matchedLocation->getCgiExt();

    if (!_matchedLocation->isAllowedMethod(_request.getHttpMethod()))
        throw 405; // Method Not Allowed

    if (_matchedLocation->isRedirect())
        throw _matchedLocation->getRedirect()._status;

    if (cgiExt.empty() || !Util::endsWith(path, cgiExt))
        return doNonCgiProcess(method); // return statusCode, throw 405
    else
    {
        cgiProcess(method, cgiExt); // throw 405, 500
        _status = EXEC_CGI;
        return 0;
    }
}

/**
 * @brief
 *
 * @param root
 * @param path
 * @param autoindex
 * @return int statusCode: Response 응답에 사용 될 값이다.
 */
int Client::notCgiGetProcess(const std::string &root, const std::string &path, bool autoindex, const std::vector<std::string> &index)
{
    int statusCode;

    try
    {
        const std::string &filepath = Util::getRootedPath(path, _matchedLocation->getUri(), root);
        std::string fileData = File::getFile(path, filepath, autoindex, index);
        _response.setBody(fileData);
        statusCode = 200;
        _erron = 0;
        _defaultBodyNeed = 0; // 바디는 fileData
    }
    catch (int errorStatusCode)
    {
        statusCode = errorStatusCode;
        _erron = 1;           // _erron 삭제 해도 될 수도
        _defaultBodyNeed = 1; // 에러는 기본 바디 필요
    }
    return statusCode;
}

/**
 * @brief
 *
 * @param filepath
 * @param body
 * @return int statusCode: Response 응답에 사용 될 값이다.
 */
int Client::notCgiPostProcess(const std::string &filepath, const std::string &body)
{
    int statusCode;

    try
    {
        File::uploadFile(filepath, body);
        statusCode = 201; // created
        _erron = 0;
        _defaultBodyNeed = 1; // Post는 바디 콘텐츠 X
    }
    catch (int errorStatusCode)
    {
        statusCode = errorStatusCode;
        _erron = 1;
        _defaultBodyNeed = 1;
    }
    return statusCode;
}

/**
 * @brief
 *
 * @param filepath
 * @return int statusCode: Response 응답에 사용 될 값이다.
 */
int Client::notCgiDeleteProcess(const std::string &filepath)
{
    int statusCode;

    try
    {
        File::deleteFile(filepath);
        statusCode = 200;
        _erron = 0;
        _defaultBodyNeed = 1; // Delete는 바디 콘텐츠 X
    }
    catch (int errorStatusCode)
    {
        statusCode = errorStatusCode;
        _erron = 1;
        _defaultBodyNeed = 1;
    }
    return statusCode;
}

/**
 * 리스폰스 세팅
 * 1.1 _response.헤더<Cotent-length && Date>세팅(리스폰스.바디, 현재:Date)
 * 1.2 리다이렉트 => Location 헤더 설정
 * 2. _response.generateResponseData() 호출: Response가 가진것들을 활용해 _data로 만듬.
 */
void Client::makeResponseData(int statusCode, int defaultBodyNeed)
{
    // 두 인자를 새롭게 받아 와서 활용.
    _response.setStatusCode(statusCode);

    if (defaultBodyNeed)
        _response.setBody(createDefaultPage(statusCode));

    // 1.1 _response.헤더<Cotent-length && Date>세팅(리스폰스.바디, 현재:Date)
    _response.setHeader(std::string("Content-Length"), std::string(Util::ft_itoa(_response.getBody().length())));
    _response.setHeader(std::string("Date"), Util::getDateString());

    // Q. 리다이렉트는 어떻게 작동되는걸까?
    // 1.2 300번대 리다이렉트
    if (statusCode / 100 == 3) // _matchedLocation && _matchedLocation->isRedirect()
    {
        _response.setHeader(std::string("Location"),
                            std::string("http://") + _request.findHeaderValue(std::string("host")) + _matchedLocation->getRedirectUrl(_request.getRequestUrl()));
    }

    // 2. _response.generateResponseData() 호출: Response가 가진것들을 활용해 _data로 만듬.
    _response.generateResponseData();
}

std::string Client::createDefaultPage(int statusCode)
{
    const std::vector<t_status_page> &errorPages = _matchedLocation->getErrorPage();
    std::string html;

    try
    {
        for (std::vector<t_status_page>::const_iterator it = errorPages.begin();
            it != errorPages.end(); ++it)
        {
            for (size_t i = 0; i < it->_status.size(); ++i)
            {
                if (it->_status[i] != statusCode)
                    continue;                    
                
                const std::string &root = _matchedLocation->getRoot();
                const std::string &filepath = root + it->_page.substr(1);
                
                std::cout << RED << "디폴트에러페이지: " << filepath << RESET << std::endl;

                html = File::getOnlyFile(filepath); // throw statusCode
                
                return html;
            }
        }

        html = createDefaultBody(statusCode);

    }
    catch (int errorStatusCode)
    {
        _response.setStatusCode(errorStatusCode);
        html = createDefaultBody(errorStatusCode);
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
    if (DEBUG_PRINT || true)
        std::cout << GREEN << "------- sendProcess -------" << RESET << std::endl;

    ssize_t bytes_sent = send(_socket, _response.getData().c_str(), _response.getDataLength(), 0);

    if (bytes_sent == -1)
    {
        // MSG_NOSIGNAL가 없어서 signal(SIGPIPE, SIG_IGN);를 통해서 SIGPIPE가 들어와도 프로세스가 종료되지 않도록.
        // 클라이언트에서 먼저 종료시: Broken pipe
        std::cerr << "Fail to send(): " + std::string(strerror(errno)) << std::endl;
        _ws->closeClient(*this);
        return;
    }

    // if (DEBUG_PRINT)
    // {
    //     std::cout << "====================== 보낸 데이터 (응답 확인용) ===============" << std::endl;
    //     std::cout << _response.getData().substr(0, bytes_sent) << std::endl;
    //     std::cout << "====================== ********* ===============" << std::endl;
    // }

    _response.updateData(bytes_sent);
    _response.updateSendedBytes(bytes_sent);

    if (DEBUG_PRINT || true)
        std::cout << "send: " << bytes_sent << " bytes, (" << _response.getSendBytes() << "/" << _response.getTotalBytes() << ")" << std::endl; // 마지막 테스트: 100000140

    if (checkSendBytes() >= 0)
    {
        if (!(DEBUG_PRINT || true)) std::cout << GREEN << "------- sendProcess -------" << RESET << std::endl;
        std::cout << RED << "전송완료: " << _request.getHttpMethod() << " " << _request.getRequestPath() << " " << _response.getStatusCode() << RESET << std::endl;
        if (!(DEBUG_PRINT || true)) std::cout << RED << "send: " << bytes_sent << " bytes, (" << _response.getSendBytes() << "/" << _response.getTotalBytes() << RESET << ")" << std::endl;

        if (_response.getStatusCode() > 400 && _response.getStatusCode() < 500)
        {
            _ws->closeClient(*this);
            return;
        }

        _eventHandler->switchToReadState(_socket);
        if (_status == READ_POST_EXPECT_100)
        {
            if (DEBUG_PRINT)
                std::cout << RED << "100 응답 전송 후" << RESET << std::endl;
            _status = READ_HEADER;
            _response.clean();
            _response.setHttpVersion(_request.getHttpVersion());
            // _eventHandler->registerTimerEvent(_socket, TIMER_TIME_OUT_SEC);
            return;
        }
        _eventHandler->registerTimerEvent(_socket, TIMER_KEEP_ALIVE_SEC);
        cleanClientRequestReponse();
    }

    return;
}

/** return (보낸바이트 - 보내야할바이트), 0 >= 0 이면 송신 끝을 의미 **/
int Client::checkSendBytes() const { return _response.getSendedBytes() - _response.getTotalBytes(); }

/**
 * @brief _matchedHost와 _matchedLocation가 결정된다!
 *
 * @throws int statusCode (400: about Host헤더, 405: Location에서 허용하지 않은 메서드, 30x: 정상적인 리다이렉트, 417: POST && 100 불가)
 */
void Client::handleHeaders(void)
{
    if (DEBUG_PRINT)
        std::cout << "========= handleHeaders ===========" << std::endl;

    const std::string POST_METHOD("POST");

    std::string hostname;
    bool expected100 = false;

    _request.handleHeaders(hostname, expected100);

    if (hostname.empty())
    {
        std::cerr << "hostname empty 400 error" << std::endl;
        throw 400;
    }

    std::cout << "hostname: " << hostname << std::endl;

    sessionProcess();

    if (DEBUG_PRINT)
        std::cout << "========= matching host & location ===========" << std::endl;

    _matchedHost = _server->matchHost(hostname);
    _matchedLocation = _matchedHost->matchLocation(_request.getRequestPath());

    const std::string &method = _request.getHttpMethod();

    // POST 100 응답
    if (expected100 && method.compare(POST_METHOD) == 0)
    {
        const std::string &cgiExt = _matchedLocation->getCgiExt();

        const std::string &root = _matchedLocation->getRoot();
        const std::string &path = _request.getRequestPath();
        const std::string &uri = _matchedLocation->getUri();

        if (cgiExt.empty() || !Util::endsWith(path, cgiExt))
        {
            // Non-CGI
            const std::string &filepath = root + Util::extractBasename(path);
            // const std::string &filepath = root + "/" + _request.getRequestPath().substr(uri.length());
            int statusCode = File::canUploadFile(filepath);
            if (statusCode)
            {
                std::cerr << "100 체크 => 업로드 불가: " << statusCode << std::endl;
                throw 417; // Expectation Failed
            }
        }
        else
        {
            // CGI
            const std::string &filepath = Util::getRootedPath(path, uri, root);
            int statusCode = canExcuteCgi(method, cgiExt, filepath);
            if (statusCode)
            {
                std::cerr << "100 체크 => CGI 실행 불가: " << statusCode << std::endl;
                throw 417; // Expectation Failed
            }
        }
        _status = READ_POST_EXPECT_100;
    }
}
/**
 * @brief
 *
 * @throws int statusCode (413: 클라이언트 바디 제한 초과, 400: 청크읽기 과정에서 에러 발생)
 *
 * \verbatim
 * chunked-body   = *chunk
 *                 last-chunk
 *                 trailer-part
 *                 CRLF
 *
 * chunk          = chunk-size [ chunk-ext ] CRLF
 *                  chunk-data CRLF
 * chunk-size     = 1*HEXDIG
 * last-chunk     = 1*("0") [ chunk-ext ] CRLF
 *
 * chunk-data     = 1*OCTET ; a sequence of chunk-size octets
 *
 * chunk-ext      = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
 * chunk-ext-name = token
 * chunk-ext-val  = token / quoted-string
 *
 * trailer-part   = *( header-field CRLF )
 * \endverbatim
 */
void Client::chunkRead(void)
{
    const size_t clientMaxBodySize = _matchedLocation->getClientMaxBodySize();
    std::size_t chunkSize, octecPos;

    try
    {
        while (!Util::isLastChunk(_request.getRawData()))
        {
            if ((chunkSize = Util::tryReadChunk(_request.getRawData(), octecPos)) == 0)
                return; // 추가 read가 필요함

            _request.getChunkOctetData().append(_request.getRawData().substr(octecPos, chunkSize));
            _request.setRawData(_request.getRawData().substr(octecPos + chunkSize + CRLF_SIZE));

            // std::cout << RED << "_chunkOctetData.length: " << _request.getChunkOctetData().length() << RESET << std::endl;

            if (_request.getChunkOctetData().size() > clientMaxBodySize)
                throw 413;
        }

        // LastChunk를 찾아 끝내도 됨.

        if (DEBUG_PRINT)
            std::cout << BLUE << "lastchunk 발견: " << _request.getRawData() << RESET << std::endl;
        if (DEBUG_PRINT)
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

void Client::cgiProcess(const std::string &method, const std::string &cgiExt)
{
    const bool isNeedFileCheck = false;
    std::string programPath;
    std::vector<std::string> argv;

    // 1. 지원하는 Method인지 체크 => CGI는 GET과 POST만 지원
    if (method.compare("GET") && method.compare("POST"))
        throw 405; // 405 Method Not Allowed,

    const std::string &path = _request.getRequestPath();
    const std::string &uri = _matchedLocation->getUri();
    const std::string &root = _matchedLocation->getRoot();
    const std::string &filepath = Util::getRootedPath(path, uri, root);

    if (isNeedFileCheck && !File::checkFileExist(filepath))
        throw 404;

    // 2. 지원하는 확장자인지 체크 && 각 실행에 필요한 2가지 요소 세팅

    if (cgiExt.compare(".bla") == 0)
    {
        programPath = "tester/cgi_tester";
        // Nothing to do for argv;
    }
    else if (cgiExt.compare(".py") == 0)
    {
        programPath = "/usr/bin/python3";
        argv.push_back(std::string("/usr/bin/python3"));
        argv.push_back(filepath);
    }
    else if (cgiExt.compare(".php") == 0)
    {
        programPath = "/usr/bin/php";
        argv.push_back(std::string("/usr/bin/php"));
        argv.push_back(filepath);
    }
    else
    {
        throw 501; // Not Implemented
    }

    // 3. 존재 && 권한체크(filepath)
    if (File::canExecuteFile(programPath))
    {
        std::cerr << RED << "503: 존재 & 권한 체크 실패: " <<  programPath << RESET << std::endl;
        throw 503; // Service Unavailable, canExecuteFile()의 statusCode이 아닌, 적절한 의미를 가진 500번대 상태코드 사용
    }

    // 4. data 세팅
    if (method.compare("POST") == 0)
    {
        _cgi.setPostData(_request.getPostData());
        std::cout << "cgi data: " << _cgi.getPostData().size() << std::endl;
    }

    try
    {
        _cgi.setEnvFromRequestHeaders(_request, method, path);
        _cgi.exec(method, programPath, argv);
    }
    catch (const char *msg)
    {
        std::cerr << "cgiProcess 실패: " << msg << std::endl;
        _cgi.clearCgi();
        throw 500;
    }

    // 7. 이벤트 등록
    if (method.compare("POST") == 0 && !_cgi.getPostData().empty())
        _eventHandler->addKeventToChangeList(_cgi.getInPipe(WRITE_FD), EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    _eventHandler->addKeventToChangeList(_cgi.getOutPipe(READ_FD), EVFILT_READ, EV_ADD, 0, 0, NULL);
}

void Client::makeCgiResponse()
{
    int statusCode = 200;

    _cgi.closePipe(_cgi.getOutPipe(READ_FD));

    const std::string &readData = _cgi.getReadData();

    // CGI 테스트시 주석 해제하면 유용함
    // std::cout << BLUE << "======== readData ===========" << RESET << std::endl;
    // std::cout << RED << readData << RESET << std::endl;
    // std::cout << BLUE << "========= . .. ..  . . ==========" << RESET << std::endl;

    size_t pos = readData.find("\r\n\r\n");
    std::string header;
    
    if (pos == std::string::npos)
        throw "CGI 응답에서 더블CRLF를 찾지 못했습니다.";

    _response.setBody(readData.substr(pos + 4));
    header = readData.substr(0, pos);

    handleCgiHeaders(header, statusCode); // 꼭 사용해야할것은 Status 헤더

    if (DEBUG_PRINT)
    {
        std::cout << YELLOW<< "================ CGI의 응답 생성 ==============" << std::endl;
        
        std::cout << GREEN << "_cgi.getPostData().size() :" << _cgi.getPostData().size() << std::endl; // 1억
        std::cout << GREEN << "_cgi.getSendBytes() :" << _cgi.getSendBytes() << std::endl; // 1억 
        std::cout << GREEN << "_readData.size() :" << readData.size() << std::endl; // 1억 + 58
        
        std::cout << YELLOW << "************** 헤더 *************************" << std::endl;
        
        if (pos) std::cout << readData.substr(0, pos) << std::endl;
        else std::cout << "No 헤더" << std::endl;
        
        std::cout << BLUE << "_cgi.getPostData().size(): " << _cgi.getPostData().size() << std::endl;
        std::cout << BLUE << "_response.getBody().size(): " << _response.getBody().size() << std::endl;

        std::cout << YELLOW<< "================ ******** ==============" << RESET  << std::endl;
    }

    makeResponseData(statusCode, 0);

    _eventHandler->turnOnWrite(_socket);

    _cgi.clearCgi();
}

void Client::makeCgiErrorResponse()
{
    makeResponseData(500, 1);

    _eventHandler->turnOnWrite(_socket);

    _cgi.clearCgi();
}

bool Client::isPipe(int fd) { return _cgi.isPipe(fd); }

void Client::clean()
{
    _contentLength = 0;

    _status = BEFORE_READ;
    _ischunk = 0;
    _erron = 0;
    _defaultBodyNeed = 0;

    _matchedHost = 0;
    _matchedLocation = 0;
}

void Client::cleanRequestReponse(void)
{
    _request.resetRequest();
    _response.clean();
}

void Client::cleanClientRequestReponse(void)
{
    clean();
    cleanRequestReponse();
}

void Client::cleanCgi()
{
    _cgi.clearCgi();
}

void Client::cleanAll()
{
    cleanCgi();
    cleanClientRequestReponse();
}

void Client::cleanForClose()
{
    cleanCgi();
}

static bool isValidSession(std::map<std::string, t_session> &sessions, std::map<std::string, t_session>::iterator sessionIt, std::time_t currentTime)
{
    return (sessionIt != sessions.end() && currentTime < sessionIt->second.expirationTime);
}

void Client::sessionProcess()
{
    if (DEBUG_PRINT)
        std::cout << "========= cookie check ===========" << std::endl;
    // ****** 쿠키 자리 ******** //
    std::map<std::string, t_session> &sessions = _ws->getSessions();
    std::map<std::string, t_session>::iterator sessionIt = sessions.end();

    const std::string &cookieValue = _request.findHeaderValue("Cookie");

    if (!cookieValue.empty())
    {
        if (DEBUG_PRINT)
            std::cout << "========= parse Cookies ===========" << std::endl;
        std::map<std::string, std::string> cookies = parseCookies(cookieValue);
        std::map<std::string, std::string>::iterator it = cookies.find("sessionid");
        if (it != cookies.end())
            sessionIt = sessions.find(it->second);
        // if (DEBUG_PRINT) std::cout << "it: " << std::endl;
    }

    std::time_t currentTime = std::time(0);
    if (!isValidSession(sessions, sessionIt, currentTime))
    {
        if (DEBUG_SESSION_PRINT)
        {
            if (sessionIt == sessions.end())
                std::cout << "no id" << std::endl;
            else
                std::cout << "curr " << currentTime << " bigger than expire " << sessionIt->second.expirationTime << std::endl;
        }
        if (sessionIt != sessions.end())
            sessions.erase(sessionIt->first);

        const std::string &sessionId = generateSessionId(sessions);

        sessions[sessionId] = t_session(sessionId, 1, currentTime + COOKIE_EXPIRE_SEC);

        _response.addCookie("sessionid", sessionId, COOKIE_EXPIRE_SEC);
        
        if (DEBUG_SESSION_PRINT)
            std::cout << "세션 시작: id(" << sessions[sessionId].id
                      << "), count(" << sessions[sessionId].count
                      << "), expirationTime(" << sessions[sessionId].expirationTime << ")" << std::endl;
    }
    else
    {
        sessionIt->second.count++;
        sessionIt->second.expirationTime = currentTime + COOKIE_EXPIRE_SEC;

        _response.addCookie("sessionid", sessionIt->second.id, COOKIE_EXPIRE_SEC);

        if (DEBUG_SESSION_PRINT)
            std::cout << "카운트 +1: id(" << sessionIt->second.id
                      << "), count(" << sessionIt->second.count
                      << "), expirationTime(" << sessionIt->second.expirationTime << ")" << std::endl;
    }
    if (DEBUG_SESSION_PRINT)
        std::cout << "sessions.size(): " << sessions.size() << std::endl;
}