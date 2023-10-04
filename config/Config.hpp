#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <string>
#include <vector>

#include "JsonParser.hpp"

typedef struct s_status_page
{
	int			_status;
	std::string	_page;

	s_status_page(void): _status(-1) {}
}	t_status_page;

typedef struct s_location
{
	std::string					m_path; // must be unique
	std::string					_root;
	std::string					_client_max_body_size;
	std::string					_cgi;
	bool						_autoindex;
	std::vector<std::string>	_limit_except;
	std::vector<std::string>	_index;
	std::vector<t_status_page>	_error_page;
	s_status_page					_return;

	s_location(void): _autoindex(false) {}
}	t_location;

typedef struct s_host
{
	int							m_listen; // must be unique
	std::string					m_server_name; // must be unique
	std::string					_root;
	std::vector<std::string>	_index;
	std::vector<t_location>		_locations;

	s_host(void): m_listen(-1) {}
}	t_host;

class	Config
{
	private:
		JsonData			_json;
		std::vector<t_host>	_hosts;

	public:
		Config(void);
		~Config(void);

	public:
		std::vector<t_host> const&	getHosts(void) const;
		void						setJson(JsonData& json);
		void						setUpHosts(void);
};

#endif	/* __CONFIG_HPP__ */
