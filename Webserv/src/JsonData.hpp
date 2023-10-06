#ifndef __JSONDATA_HPP__
#define __JSONDATA_HPP__

#include <string>
#include <map>
#include <vector>

enum jsonType
{
	TYPE_INTEGER,
	TYPE_FLOAT,
	TYPE_STRING,
	TYPE_BOOLEAN,
	TYPE_ARRAY,
	TYPE_OBJECT,
	TYPE_NULL,
	TYPE_ERROR
};

class JsonData
{
	public:
		typedef std::pair<std::string, JsonData>	kv;

	public:
		jsonType					_type;
		std::string					_str;
		std::vector<JsonData>		_arr;
		std::vector<JsonData::kv>	_obj;

	public:
		JsonData(void);
		JsonData(JsonData const& target);
		JsonData&	operator=(JsonData const& target);
		~JsonData(void);
	
	public:
		jsonType						getJsonDataType() const;
		const std::string				&getStringData() const;
		const std::vector<JsonData>		&getArrData() const;
		const std::vector<kv>			&getObjData() const;
};

#endif	/* __JSONDATA_HPP__ */
