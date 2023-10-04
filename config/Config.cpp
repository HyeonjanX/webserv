#include "Config.hpp"

/* *************************************************************************** *
 * Constructor & Destructor                                                    *
 * ****************************************************************************/

Config::Config(void) {}

Config::~Config(void) {}

/* *************************************************************************** *
 * Public Member Functions                                                     *
 * ****************************************************************************/

std::vector<t_host> const&	Config::getHosts(void) const
{
	return this->_hosts;
}

void	Config::setJson(JsonData& json)
{
	this->_json = json;
}

void	Config::setUpHosts(void)
{
	std::vector<t_host>		hosts;
	std::vector<JsonData>	servers;

	servers = jsonParser.findDataByKey(this->_json, "server");
	for (std::vector<JsonData>::iterator sit = servers.begin();
		sit != servers.end(); ++it)
	{
		std::vector<JsonData>	ret;
		t_host					host;

		ret = jsonParser.findDataByKey(*sit, "listen");
	}
}

// wwowowowowowowowoow

