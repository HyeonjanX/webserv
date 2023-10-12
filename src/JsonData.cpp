#include "JsonData.hpp"

JsonData::JsonData(void) : _type(TYPE_NULL) {}

JsonData::JsonData(JsonData const& target)
{
	if (this != &target)
		*this = target;
}

JsonData&	JsonData::operator=(JsonData const& target)
{
	if (this != &target)
	{
		this->_type = target._type;
		this->_str = target._str;
		this->_arr = target._arr;
		this->_obj = target._obj;
	}
	return *this;
}

JsonData::~JsonData(void) {}

jsonType						JsonData::getJsonDataType() const { return _type; }
const std::string				&JsonData::getStringData() const { return _str; }
const std::vector<JsonData>		&JsonData::getArrData() const { return _arr; }
const std::vector<JsonData::kv>	&JsonData::getObjData() const { return _obj; }
