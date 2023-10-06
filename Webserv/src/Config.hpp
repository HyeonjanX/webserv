#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <string>
#include <vector>

#include "JsonParser.hpp"

typedef struct s_status_page
{
    std::vector<int>			_status;
    std::string                 _page;

}	t_status_page;

typedef struct s_redirect
{
    int			                _status;
    std::string                 _page;

    s_redirect(void): _status(0) {}
}	t_redirect;

typedef struct s_location
{
    std::string                 m_path; // must be unique
    std::string                 _root;
    std::size_t                 _client_max_body_size;
    std::string                 _cgi;
    bool                        _autoindex;
    std::vector<std::string>    _limit_except;
    std::vector<std::string>    _index;
    std::vector<t_status_page>  _error_page;
    t_redirect                  _return;

    s_location(void): _root("."), _client_max_body_size(1000000), _autoindex(false) {
        _index.push_back("index.html");
    }
}	t_location;

typedef struct s_host
{
    int                        	_listen;
    std::string                 _server_name;
    std::size_t                 _client_max_body_size;
    std::string                 _root;
    std::vector<std::string>    _index;
    std::vector<t_status_page>  _error_page;
    std::vector<t_location>     _locations;

    s_host(void): _listen(80) {}
}	t_host;

class	Config
{
    private:
        JsonData                        _json;
        std::vector<t_host>				_hosts;
        std::map<std::string, jsonType> _serverDirectives;  // 서버 지시어
        std::map<std::string, jsonType> _locationDirectives;  // 로케이션 지시어

    public:
        Config(void);
        ~Config(void);

    public:
        std::vector<t_host> const&	getHosts(void) const;
        void                        setJson(const JsonData& json);
        void                        setUpHosts(void);

        void                        initializeDirectives();  // 지시어 초기화 함수
        bool                        isValidServerDirective(const std::string &key, jsonType type);
        bool                        isValidLocationDirective(const std::string &key, jsonType type);

        std::map<int, std::vector<t_host> > makeServerConfigs(void);
    
        void                        parseServer(std::map<int, std::vector<t_host> > &servers, const std::vector<JsonData::kv> &serverKeyValues);
        void                        parseLocation(t_host &host, const std::vector<JsonData::kv> &serverKeyValues);

        // 파싱 중복 체크
        void                        serverIdentyIsDuplicated(const std::vector<t_host> &hosts, const t_host &host);
        void                        locationPathIsDuplicated(const std::vector<t_location> &locations, const t_location &location);

        // 서버 파싱
        void                        serverParseListen(t_host &host, const JsonData &value);
        void                        serverParseServerName(t_host &host, const JsonData &value);
        void                        serverParseClientMaxBodySize(t_host &host, const JsonData &value);
        void                        serverParseRoot(t_host &host, const JsonData &value);
        void                        serverParseIndex(t_host &host, const JsonData &value);
        void                        serverParseErrorPage(t_host &host, const JsonData &value);
    
        // 로케이션 파싱
        void                        locationParsePath(t_location &location, const JsonData &value);
        void                        locationParseRoot(t_location &location, const JsonData &value);
        void                        locationParseClientMaxBodySize(t_location &location, const JsonData &value);
        void                        locationParseCgi(t_location &location, const JsonData &value);
        void                        locationParseAutoIndex(t_location &location, const JsonData &value);
        void                        locationParseLimitExcept(t_location &location, const JsonData &value);
        void                        locationParseIndex(t_location &location, const JsonData &value);
        void                        locationParseErrorPage(t_location &location, const JsonData &value);
        void                        locationParseReturn(t_location &location, const JsonData &value);

        // 출력 확인
        void                        printHosts(const std::vector<t_host> &hosts);
};

#endif	/* __CONFIG_HPP__ */
