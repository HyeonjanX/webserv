#include "JsonParser.hpp"
#include "Config.hpp"
#include "util.hpp"

int	main(int argc, char *argv[])
{
	JsonParser	jsonParser;
	Config		config;

//	atexit(check_leaks);
	if (argc == 2)
	{
		std::vector<JsonData>	ret;

		config.setJson(jsonParser.parseJson(argv[1]));
		std::cout << "====== result =====" << std::endl;
		printJson(jsonParser.getJson(), 0);

		std::cout << "====== all server block ======" << std::endl;
		ret = jsonParser.findDataByKey(jsonParser.getJson(), "server");
		printJsonArray(ret, 0);

		std::cout << "====== all location block ======" << std::endl;
		for (std::vector<JsonData>::iterator it = ret.begin();
			it != ret.end(); ++it)
		{
			std::vector<JsonData>	elem;

			elem = jsonParser.findDataByKey(*it, "location");
			printJsonArray(elem, 0);
			std::cout << std::endl;
			std::cout << "===========" << std::endl;
		}

	}
	else
	{
		std::cerr << "Please test with file" << std::endl;
		return 1;
	}
	return 0;
}
