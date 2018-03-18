#include <iostream>
#include <sstream>
#include "StringTools.h"

namespace Utils
{
	void tokenize(const std::string& str, char delimiter, std::vector<std::string>& output_tokens_list)
	{
		int size = str.size();
		int start = 0, end = 0;
		while (end < size)
		{
			if (str[end] == delimiter && start <= end)
			{
				output_tokens_list.push_back(std::string(str.substr(start, end - start + 1)));
				start = end + 1;
			}
			end++;
		}
		if (str[end - 1] != delimiter)
			output_tokens_list.push_back(std::string(str.substr(start, end - start)));
	}
}