#ifndef XMLWRITE_H
#define XMLWRITE_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace Utils
{
	class XmlWriter {
	public:
		bool Open(const std::string);
		void Close();
		bool exists(const std::string);
		void Write_open_tag(const std::string);
		void Write_close_tag();
		void Write_start_element_tag(const std::string);
		void Write_end_element_tag();
		void Write_attribute(const std::string);
		void Write_string(const std::string);
		void Write_attribute_string(const std::string attribute_name, const std::string attribute_value);
		void Write_attribute_string_inline(const std::string attribute_name, const std::string attribute_value);
	private:
		std::ofstream outFile;
		int indent;
		int openTags;
		int openElements;
		std::vector<std::string> tempOpenTag;
		std::vector<std::string> tempElementTag;
	};
}

#endif // !XMLWRITE_H
