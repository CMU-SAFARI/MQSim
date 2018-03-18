#ifndef STRING_TOOLS_H
#define STRING_TOOLS_H

#include <string>
#include <vector>

namespace Utils
{
	void tokenize(const std::string& str, char delimiter, std::vector<std::string>& output_tokens_list);
}

#endif