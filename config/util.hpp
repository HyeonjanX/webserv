#include "JsonParser.hpp"

void	check_leaks(void);
void	printJsonArray(std::vector<JsonData> const& jsonArray, size_t depth);
void	printJson(JsonData const& jsonData, size_t depth);
