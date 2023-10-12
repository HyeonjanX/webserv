#ifndef __JSONPARSER_HPP__
#define __JSONPARSER_HPP__

#include <fstream>
#include <sstream>
#include <iostream>
#include <sstream>
#include <string>

#include "JsonData.hpp"

class JsonParser
{
	private:
		JsonData	_json;

	public:
		JsonParser(void);
		~JsonParser(void);

	public:
		JsonData const&						getJson(void) const;
		JsonData&							parseJson(
												std::string const& filepath
											);
		std::vector<JsonData>				findDataByKey(
												JsonData const& jsonData,
												std::string const& key
											);

	private:
		void								readFile(
												std::string const& filepath,
												std::string& output
											);
		std::pair<std::string, JsonData>	retriveKeyValuePair(
												std::string const& text,
												std::string::iterator& it
											);
		std::vector<JsonData>				parseArray(
												std::string const& text,
												std::string::iterator& it
											);
		JsonData							parseObject(
												std::string const& text,
												std::string::iterator& it
											);
		bool								checkElementEnd(
												std::string const& text,
												std::string::iterator& it
											);
		bool								checkKeyValueEnd(
												std::string const& text,
												std::string::iterator& it
											);
		std::string							parsePrimitive(
												std::string const& text,
												std::string::iterator& it,
												jsonType& type
											);
		std::string							parseStringKey(
												std::string const& text,
												std::string::iterator& it
											);
		std::string							parseStringValue(
												std::string const& text,
												std::string::iterator& it
											);
		std::string							getStringData(
												std::string const& text,
												std::string::iterator& it
											);
		jsonType							getPrimitiveType(
												std::string const& str
											);
};

#endif	/* __JSONPARSER_HPP__ */
