#include "Config.hpp"
#include "Util.hpp"

#define DEBUG_PRINT false

/* *************************************************************************** *
 * Constructor & Destructor                                                    *
 * ****************************************************************************/

Config::Config(void) { initializeDirectives(); }

Config::~Config(void) {}

/* *************************************************************************** *
 * Static Helper Functions                                                     *
 * ****************************************************************************/


// static std::vector<std::string> split(const std::string &str, const std::string &delimiter)
// {
// 	std::vector<std::string> result;

// 	size_t wordPos, oldPos = 0;

// 	while ((wordPos = str.find_first_not_of(delimiter, oldPos)) != std::string::npos)
// 	{
// 		size_t delPos = str.find_first_of(delimiter, wordPos + 1);
// 		if (delPos != std::string::npos)
// 			result.push_back(str.substr(wordPos, delPos - wordPos));
// 		else
// 		{
// 			result.push_back(str.substr(wordPos));
// 			break;
// 		}
// 		oldPos = delPos;
// 	}
// 	return result;
// }

static long getMeasurementSize(const std::string &measure)
{
	long size;
	char unit;
	char *endptr = NULL;
	size_t len;
	size_t scale;

	len = measure.length();
	if (!len)
		return -1;

	unit = measure[len - 1];
	if (unit == 'k' || unit == 'K')
	{
		len--;
		scale = 1024;
	}
	else if (unit == 'm' || unit == 'M')
	{
		len--;
		scale = 1024 * 1024;
	}
	else if (std::isdigit(unit))
	{
		scale = 1;
	}
	else
	{
		return -1;
	}

	std::string sub = measure.substr(0, len);

	size = strtol(sub.c_str(), &endptr, 10);
	if (*endptr != '\0')
		return -1;

	size *= scale;

	if (size > INT_MAX)
		return -2;
	else if (size < 0)
		return -3;

	return size;
}

static bool	isValidUriString(const std::string &url)
{
	for (size_t i = 0; i < url.length(); ++i)
	{
		char c = url[i];

		if (!((std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~' ||
			c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']' ||
			c == '@' || c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' ||
			c == ')' || c == '*' || c == '+' || c == ',' || c == ';' || c == '%' || c == '=')))
		{
			return false;
		}
	}
	return true;
}

static bool	isInternetUrl(const std::string &url)
{
	if (Util::startsWith(url, "http://") || Util::startsWith(url, "https://"))
	{
		return true;
	}
	return false;
}

static bool	isValidDirectoryPath(const std::string &url, char mark)
{
	if (url.empty())
	{
		return false;
	}

	if (mark == 'P' && (url[0] != '/' || url[url.length() - 1] != '/'))
	{
		return false;
	}
	else if (mark == 'R' && (url[url.length() - 1] != '/'))
	{
		return false;
	}
	else if (mark == 'r' && (url[0] != '/'))
	{
		return false;
	}

	// 연속되는 슬래쉬 방지
	for (std::size_t i = 1; i < url.length(); ++i)
	{
		if (url[i] == '/' && url[i - 1] == '/')
		{
			return false;
		}
	}
	return true;
}

static bool isConvertibleToInt(const std::string &str, int &outNum)
{
	std::stringstream ss(str);
	ss >> outNum;

	// 변환 실패 또는 끝까지 읽지 않았을 경우 false 반환
	return !ss.fail() && (ss.eof() || ss.peek() == EOF);
}

/* *************************************************************************** *
 * Public Member Functions                                                     *
 * ****************************************************************************/

std::vector<t_host> const &Config::getHosts(void) const
{
	return this->_hosts;
}

void Config::setJson(const JsonData &json)
{
	this->_json = json;
}

void Config::setUpHosts(void)
{
	std::vector<t_host> hosts;
	std::vector<JsonData> servers;

	// servers = jsonParser.findDataByKey(this->_json, "server");
	// for (std::vector<JsonData>::iterator sit = servers.begin();
	// 	sit != servers.end(); ++it)
	// {
	// 	std::vector<JsonData>	ret;
	// 	t_host					host;

	// 	ret = jsonParser.findDataByKey(*sit, "listen");
	// }
}

void Config::initializeDirectives()
{
	// 서버 지시어 초기화
	_serverDirectives["listen"] = TYPE_INTEGER;
	_serverDirectives["server_name"] = TYPE_STRING;
	_serverDirectives["root"] = TYPE_STRING;
	_serverDirectives["client_max_body_size"] = TYPE_STRING;
	_serverDirectives["error_page"] = TYPE_ARRAY;
	_serverDirectives["index"] = TYPE_ARRAY;
	_serverDirectives["location"] = TYPE_OBJECT;

	// 로케이션 지시어 초기화
	_locationDirectives["path"] = TYPE_STRING;
	_locationDirectives["root"] = TYPE_STRING;
	_locationDirectives["client_max_body_size"] = TYPE_STRING;
	_locationDirectives["cgi"] = TYPE_STRING;
	_locationDirectives["autoindex"] = TYPE_BOOLEAN;
	_locationDirectives["limit_except"] = TYPE_ARRAY;
	_locationDirectives["index"] = TYPE_ARRAY;
	_locationDirectives["error_page"] = TYPE_ARRAY;
	_locationDirectives["return"] = TYPE_ARRAY;
}

bool Config::isValidServerDirective(const std::string &key, jsonType type)
{
	std::map<std::string, jsonType>::iterator it = _serverDirectives.find(key);

	return it != _serverDirectives.end() && it->second == type;
}

bool Config::isValidLocationDirective(const std::string &key, jsonType type)
{
	std::map<std::string, jsonType>::iterator it = _locationDirectives.find(key);

	return it != _locationDirectives.end() && it->second == type;
}

std::map<int, std::vector<t_host> > Config::makeServerConfigs()
{
	std::map<int, std::vector<t_host> > serverConfigs;

	if (_json.getJsonDataType() != TYPE_OBJECT)
		throw "jsonConfig는 TYPE_OBJECT여야만 합니다.";

	const std::vector<JsonData::kv> &serverObjects = _json.getObjData();

	for (std::vector<JsonData::kv>::const_iterator it = serverObjects.begin(); it != serverObjects.end(); ++it)
	{
		if (it->first.compare("server") != 0)
			throw "server 만이 가능합니다.";
		if (it->second.getJsonDataType() != TYPE_OBJECT)
			throw "server는 TYPE_OBJECT여야만 합니다.";
		parseServer(serverConfigs, it->second.getObjData());
	}

	if (DEBUG_PRINT)
	{
		for (std::map<int, std::vector<t_host> >::const_iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it)
		{
			std::cout << "port: " << it->first << std::endl;
			printHosts(it->second);
		}
	}

	return serverConfigs;
}

void Config::parseServer(std::map<int, std::vector<t_host> > &servers, const std::vector<JsonData::kv> &serverKeyValues)
{
	t_host host;
	bool indexExist = false;

	if (DEBUG_PRINT) std::cout << "parseServer size: " << serverKeyValues.size() << std::endl;

	for (std::vector<JsonData::kv>::const_iterator it = serverKeyValues.begin();
		 it != serverKeyValues.end();
		 ++it)
	{
		const std::string key = it->first;
		jsonType valueType = it->second.getJsonDataType();
		if (!isValidServerDirective(key, valueType))
		{
			std::cerr << "Invalid Server Directive: " << key << " & " << valueType << std::endl;
			throw "유효하지 않은 서버 지시어 key:valueType 페어입니다.";
		}
		if (key.compare("listen") == 0)
			serverParseListen(host, it->second);
		else if (key.compare("server_name") == 0)
			serverParseServerName(host, it->second);
		else if (key.compare("client_max_body_size") == 0)
			serverParseClientMaxBodySize(host, it->second);
		else if (key.compare("root") == 0)
			serverParseRoot(host, it->second);
		else if (key.compare("index") == 0)
		{
			serverParseIndex(host, it->second);
			indexExist = true;
		}
		else if (key.compare("error_page") == 0)
			serverParseErrorPage(host, it->second);
		else if (key.compare("location") == 0)
		{
			continue;
			//parseLocation(host, it->second.getObjData());
		}
		else
		{
			std::cerr << "key: " << key << std::endl;
			throw "parseServer(): 무언가 빼먹었다.";
		}
	}

	if (!indexExist)
	{	
		host._index.push_back("index.html");
	}

	bool mustLocation = false;

	for (std::vector<JsonData::kv>::const_iterator it = serverKeyValues.begin();
		 it != serverKeyValues.end();
		 ++it)
	{
		const std::string key = it->first;

		if (key.compare("location") == 0)
			parseLocation(host, it->second.getObjData());
		
		if (!host._locations.empty() && host._locations.back().m_path == "/")
			mustLocation = true;
	}

	if (!mustLocation)
	{
		std::cerr << "/ 로케이션은 필수로 설정해주어야 합니다." << std::endl;
		throw "/ 로케이션은 필수로 설정해주어야 합니다.";
	}

	std::map<int, std::vector<t_host> >::iterator it = servers.find(host._listen);

	if (DEBUG_PRINT) std::cout << "현재 ID: " << host._listen << host._server_name << std::endl;
	if (it == servers.end())
		servers[host._listen] = std::vector<t_host>(1, host);
	else
	{
		serverIdentyIsDuplicated(it->second, host);
		servers[host._listen].push_back(host);
	}

}

void Config::parseLocation(t_host &host, const std::vector<JsonData::kv> &locationKeyValues)
{
	t_location			location;
	enum				e_loc_status { LOC_ROOT, LOC_SIZE, LOC_INDEX, LOC_ERRPG, NUMLOCS };
	std::vector<bool>	locFlags(NUMLOCS, false);

	for (std::vector<JsonData::kv>::const_iterator it = locationKeyValues.begin();
		 it != locationKeyValues.end();
		 ++it)
	{
		const std::string key = it->first;
		jsonType valueType = it->second.getJsonDataType();

		if (!isValidLocationDirective(key, valueType))
		{
			std::cerr << "Invalid Location Directive: " << key << " & " << valueType << std::endl;
			throw "유효하지 않은 로케이션 지시어 key:valueType 페어입니다.";
		}

		if (key.compare("path") == 0)
		{
			locationParsePath(location, it->second);
		}
		else if (key.compare("root") == 0)
		{
			locationParseRoot(location, it->second);
			locFlags[LOC_ROOT] = true;
		}
		else if (key.compare("client_max_body_size") == 0)
		{
			locationParseClientMaxBodySize(location, it->second);
			locFlags[LOC_SIZE] = true;
		}
		else if (key.compare("cgi") == 0)
		{
			locationParseCgi(location, it->second);
		}
		else if (key.compare("autoindex") == 0)
		{
			locationParseAutoIndex(location, it->second);
		}
		else if (key.compare("limit_except") == 0)
		{
			locationParseLimitExcept(location, it->second);
		}
		else if (key.compare("index") == 0)
		{
			locationParseIndex(location, it->second);
			locFlags[LOC_INDEX] = true;
		}
		else if (key.compare("error_page") == 0)
		{
			locationParseErrorPage(location, it->second);
			locFlags[LOC_ERRPG] = true;
		}
		else if (key.compare("return") == 0)
		{
			locationParseReturn(location, it->second);
		}
		else
		{
			std::cerr << "key: " << key << std::endl;
			throw "parseLocation(): 무언가 빼먹었다.";
		}
	}

	if (location.m_path.empty())
		throw "location에서 path 지시어는 필수 입니다.";

	// root
	if (locFlags[LOC_ROOT] == false)
		location._root = host._root;
	
	// client max body size
	if (locFlags[LOC_SIZE] == false)
		location._client_max_body_size = host._client_max_body_size;

	// index
	if (locFlags[LOC_INDEX] == false)
		location._index = host._index;

	// error page
	if (locFlags[LOC_ERRPG] == false)
		location._error_page = host._error_page;

	locationPathIsDuplicated(host._locations, location);

	host._locations.push_back(location);
}

void Config::serverIdentyIsDuplicated(const std::vector<t_host> &hosts, const t_host &host)
{
	for (size_t i = 0; i < hosts.size(); ++i)
	{
		if (hosts[i]._server_name.compare(host._server_name) == 0)
		{
			throw "host간 identity (port + server name)은 중복이 불가합니다.";
		}
	}
}

void Config::serverParseListen(t_host &host, const JsonData &value)
{
	const int MIN_PORT = 1, MAX_PORT = 65535;

	int port;

	if (!isConvertibleToInt(value.getStringData(), port) ||
		port < MIN_PORT || port > MAX_PORT)
		throw "listen 지시어의 값이 유효한 포트 범위(1 ~ 65535)에 있지 않습니다.";

	if (DEBUG_PRINT) std::cout << "serverParseListen(): " << port << std::endl;

	host._listen = port;
}

void Config::serverParseServerName(t_host &host, const JsonData &value)
{
	host._server_name = value.getStringData();
}

void	Config::serverParseClientMaxBodySize(t_host &host, const JsonData &value)
{
	std::string clientMaxBodySize = value.getStringData();
	int size = getMeasurementSize(clientMaxBodySize);

	if (size < 0)
	{
		if (size == -1)
		{
			std::cerr << "Invalid Location client_max_body_size: " << clientMaxBodySize << std::endl;
			throw "유효하지 않은 로케이션 클라이언트 바디 사이즈입니다.";
		}
		else if (size == -2)
		{
			std::cerr << "Invalid Location client_max_body_size: " << clientMaxBodySize << std::endl;
			throw "클라이언트 바디 사이즈가 너무 큽니다.";
		}
		else if (size == -3)
		{
			std::cerr << "Invalid Location client_max_body_size: " << clientMaxBodySize << std::endl;
			throw "음수 바디 사이즈는 허용되지 않습니다.";
		}
	}

	host._client_max_body_size = static_cast<size_t>(size);
}

void Config::serverParseRoot(t_host &host, const JsonData &value)
{
	if (value.getStringData().empty())
		throw "root는 빈 문자열은 허용하지 않습니다.";
	host._root = value.getStringData();
}

void Config::serverParseIndex(t_host &host, const JsonData &value)
{
	std::vector<std::string> ret;

	const std::vector<JsonData> &arr = value.getArrData();

	for (size_t i = 0; i < arr.size(); ++i)
	{
		if (arr[i].getJsonDataType() != TYPE_STRING)
			throw "index의 모든 원소는 문자열이여야만 합니다.";
		if (arr[i].getStringData().empty() || arr[i].getStringData()[0] == '/')
			throw "index의 모든 원소는 빈 문자열이거나 /로 시작하는 절대 경로는 불가합니다.";
		ret.push_back(arr[i].getStringData());
	}
	host._index = ret;
}

void Config::serverParseErrorPage(t_host &host, const JsonData &value)
{
	t_status_page ret;

	const std::vector<JsonData> &arr = value.getArrData();

	if (arr.size() < 2)
		throw "error_page는 최소 2개의 인자로 구성되어야 합니다.";

	if (arr.back().getJsonDataType() != TYPE_STRING ||
		arr.back().getStringData().empty() ||
		arr.back().getStringData()[0] != '/')
		throw "error_page의 마지막 인자는 /로 시작하는 문자열이여야 합니다.";

	ret._page = arr.back().getStringData();

	for (size_t i = 0; i < arr.size() - 1; ++i)
	{
		int statusCode;
		if (arr[i].getJsonDataType() != TYPE_INTEGER ||
			!isConvertibleToInt(arr[i].getStringData(), statusCode) ||
			statusCode < 100 || statusCode > 599)
			throw "error_page는 마지막 원소를 제외하고 100 ~ 599 사이의 값을 가져야합니다.";
		ret._status.push_back(statusCode);
	}

	host._error_page.push_back(ret);
}

// path는 /[string]/ 형식을 따라야만 가능하게 설정
void Config::locationParsePath(t_location &location, const JsonData &value)
{
	std::string urlPath = value.getStringData();

	if ((isValidUriString(urlPath) == false)
		|| (isValidDirectoryPath(urlPath, 'P') == false))
	{
		std::cerr << "Invalid Location path: " << urlPath << std::endl;
		throw "유효하지 않은 로케이션 경로입니다.";
	}
	location.m_path = urlPath;
}

// root는 [string]/ 형식을 따라야만 가능하게 설정
void Config::locationParseRoot(t_location &location, const JsonData &value)
{
	std::string rootPath = value.getStringData();

	if ((isValidUriString(rootPath) == false)
		|| (isValidDirectoryPath(rootPath, 'R') == false))
	{
		std::cerr << "Invalid Location root: " << rootPath << std::endl;
		throw "유효하지 않은 로케이션 루트입니다.";
	}
	location._root = rootPath;
}

void Config::locationParseClientMaxBodySize(t_location &location, const JsonData &value)
{
	std::string clientMaxBodySize = value.getStringData();
	long size = getMeasurementSize(clientMaxBodySize);

	if (size < 0)
	{
		if (size == -1)
		{
			std::cerr << "Invalid Location client_max_body_size: " << clientMaxBodySize << std::endl;
			throw "유효하지 않은 로케이션 클라이언트 바디 사이즈입니다.";
		}
		else if (size == -2)
		{
			std::cerr << "Invalid Location client_max_body_size: " << clientMaxBodySize << std::endl;
			throw "클라이언트 바디 사이즈가 너무 큽니다.";
		}
		else if (size == -3)
		{
			std::cerr << "Invalid Location client_max_body_size: " << clientMaxBodySize << std::endl;
			throw "음수 바디 사이즈는 허용되지 않습니다.";
		}
	}

	location._client_max_body_size = static_cast<size_t>(size);
}

void Config::locationPathIsDuplicated(const std::vector<t_location> &locations, const t_location &location)
{
	for (size_t i = 0; i < locations.size(); ++i)
	{
		if (location.m_path.compare(locations[i].m_path) == 0)
		{
			std::cerr << "path 중복: " << location.m_path << std::endl;
			throw "server 내 location 간의 path는 중복이 불가합니다.";
		}
	}
}

void Config::locationParseCgi(t_location &location, const JsonData &value)
{
	if (value.getStringData().size() < 2 || value.getStringData()[0] != '.')
		throw "cgi는 .으로 시작하는 길이 2이상의 문자열이여야 합니다.";
	location._cgi = value.getStringData();
}

void Config::locationParseAutoIndex(t_location &location, const JsonData &value)
{ 
	location._autoindex = value.getStringData() == "true";
}

void Config::locationParseLimitExcept(t_location &location, const JsonData &value)
{
	std::vector<std::string> ret;

	const std::vector<JsonData> &arr = value.getArrData();
	for (size_t i = 0; i < arr.size(); ++i)
	{
		if (arr[i].getJsonDataType() != TYPE_STRING)
			throw "limit_except의 모든 원소는 문자열이여야만 합니다.";
		ret.push_back(arr[i].getStringData());
	}
	location._limit_except = ret;
}

void Config::locationParseIndex(t_location &location, const JsonData &value)
{
	std::vector<std::string> ret;

	const std::vector<JsonData> &arr = value.getArrData();

	for (size_t i = 0; i < arr.size(); ++i)
	{
		if (arr[i].getJsonDataType() != TYPE_STRING)
			throw "index의 모든 원소는 문자열이여야만 합니다.";
		if (arr[i].getStringData().empty() || arr[i].getStringData()[0] == '/')
			throw "index의 모든 원소는 빈 문자열이거나 /로 시작하는 절대 경로는 불가합니다.";
		ret.push_back(arr[i].getStringData());
	}
	location._index = ret;
}

void Config::locationParseErrorPage(t_location &location, const JsonData &value)
{
	t_status_page ret;

	const std::vector<JsonData> &arr = value.getArrData();

	if (arr.size() < 2)
		throw "error_page는 최소 2개의 인자로 구성되어야 합니다.";

	if (arr.back().getJsonDataType() != TYPE_STRING ||
		arr.back().getStringData().empty() ||
		arr.back().getStringData()[0] != '/')
		throw "error_page의 마지막 인자는 /로 시작하는 문자열이여야 합니다.";

	ret._page = arr.back().getStringData();

	for (size_t i = 0; i < arr.size() - 1; ++i)
	{
		int statusCode;
		if (arr[i].getJsonDataType() != TYPE_INTEGER ||
			!isConvertibleToInt(arr[i].getStringData(), statusCode) ||
			statusCode < 100 || statusCode > 599)
			throw "error_page는 마지막 원소를 제외하고 100 ~ 599 사이의 값을 가져야합니다.";
		ret._status.push_back(statusCode);
	}

	location._error_page.push_back(ret);
}

// 리디렉션되는 부분
void Config::locationParseReturn(t_location &location, const JsonData &value)
{
	t_redirect ret;

	const std::vector<JsonData> &arr = value.getArrData();

	if (arr.size() != 2)
		throw "return은 2개의 인자로 구성되어야 합니다.";

	int statusCode;

	if (arr[0].getJsonDataType() != TYPE_INTEGER ||
		!isConvertibleToInt(arr[0].getStringData(), statusCode) ||
		statusCode > 308 || statusCode < 301)
		throw "return의 첫번째 인자는 301~308사이의 정수여야 합니다.은 2개의 인자로 구성되어야 합니다.";

	ret._status = statusCode;

	if (arr[1].getJsonDataType() != TYPE_STRING ||
		arr[1].getStringData().empty() ||
		isValidUriString(arr[1].getStringData()) == false ||
		(isInternetUrl(arr[1].getStringData()) == false &&
		 isValidDirectoryPath(arr[1].getStringData(), 'r') == false))
		throw "올바르지 않은 return path 형태입니다.";

	ret._page = arr[1].getStringData();

	location._return = ret;
}

void	Config::printHosts(const std::vector<t_host> &hosts)
{	
	for (size_t i = 0; i < hosts.size(); ++i)
	{
		std::cout << "Host " << i + 1 << ":" << std::endl;
		std::cout << "Listen: " << hosts[i]._listen << std::endl;
		std::cout << "Server Name: " << hosts[i]._server_name << std::endl;
		std::cout << "Client Max Body Size: " << hosts[i]._client_max_body_size << std::endl;
		std::cout << "Root: " << hosts[i]._root << std::endl;

		std::cout << "Index Pages:" << std::endl;
		for (size_t j = 0; j < hosts[i]._index.size(); ++j)
		{
			std::cout << "  " << hosts[i]._index[j] << std::endl;
		}

		std::cout << "Locations:" << std::endl;
		for (size_t j = 0; j < hosts[i]._locations.size(); ++j)
		{
			std::cout << "  Location " << j + 1 << ":" << std::endl;
			std::cout << "    Path: " << hosts[i]._locations[j].m_path << std::endl;
			std::cout << "    Root: " << hosts[i]._locations[j]._root << std::endl;
			std::cout << "    Client Max Body Size: " << hosts[i]._locations[j]._client_max_body_size << std::endl;
			std::cout << "    CGI: " << hosts[i]._locations[j]._cgi << std::endl;
			std::cout << "    Autoindex: " << hosts[i]._locations[j]._autoindex << std::endl;

			std::cout << "    Limit Except:" << std::endl;
			for (size_t k = 0; k < hosts[i]._locations[j]._limit_except.size(); ++k)
			{
				std::cout << "      " << hosts[i]._locations[j]._limit_except[k] << std::endl;
			}

			std::cout << "    Index Pages:" << std::endl;
			for (size_t k = 0; k < hosts[i]._locations[j]._index.size(); ++k)
			{
				std::cout << "      " << hosts[i]._locations[j]._index[k] << std::endl;
			}

			std::cout << "    Error Pages:" << std::endl;
			for (size_t k = 0; k < hosts[i]._locations[j]._error_page.size(); ++k)
			{
				std::cout << "     " << "";
				for (size_t l = 0; l < hosts[i]._locations[j]._error_page[k]._status.size(); ++l)
				{
					std::cout << " " << hosts[i]._locations[j]._error_page[k]._status[l];
				}
				std::cout << hosts[i]._locations[j]._error_page[k]._page << std::endl;
			}

			std::cout << "    Redirect: " << std::endl; 
			std::cout << "        " << hosts[i]._locations[j]._return._status << " ";
			std::cout << "        " << hosts[i]._locations[j]._return._page << std::endl;
		}

		std::cout << std::endl;
	}
}
