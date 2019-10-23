#include "XMLWriter.h"
#include "../sim/Engine.h"

namespace Utils
{
	bool XmlWriter::exists(const std::string fileName) {
		std::fstream checkFile(fileName);
		return checkFile.is_open();
	}
	
	bool XmlWriter::Open(const std::string strFile) {

		outFile.open(strFile);
		if (outFile.is_open()) {
			outFile << "<?xml version=\"1.0\" encoding=\"us-ascii\"?>\n";
			indent = 0;
			openTags = 0;
			openElements = 0;

			return true;
		}

		return false;
	}
	
	void XmlWriter::Close()
	{
		if (outFile.is_open()) {
			outFile.close();
		}
	}
	
	void XmlWriter::Write_open_tag(const std::string openTag) {
		if (outFile.is_open()) {
			for (int i = 0; i < indent; i++) {
				outFile << "\t";
			}
			tempOpenTag.resize(openTags + 1);
			outFile << "<" << openTag << ">\n";
			tempOpenTag[openTags] = openTag;
			indent += 1;
			openTags += 1;
		} else {
			PRINT_ERROR("The XML output file is closed. Unable to write to file");
		}
	}
	
	void XmlWriter::Write_attribute_string(const std::string attribute_name, const std::string attribute_value)
	{
		if (outFile.is_open()) {
			for (int i = 0; i < indent + 1; i++) {
				outFile << "\t";
			}

			outFile << " <" << attribute_name + ">" + attribute_value + "</" << attribute_name + ">\n";
		} else {
			PRINT_ERROR("The XML output file is closed. Unable to write to file");
		}
	}
	
	void XmlWriter::Write_close_tag() {
		if (outFile.is_open()) {
			indent -= 1;
			for (int i = 0; i < indent; i++) {
				outFile << "\t";
			}
			outFile << "</" << tempOpenTag[openTags - 1] << ">\n";
			tempOpenTag.resize(openTags - 1);
			openTags -= 1;
		} else {
			PRINT_ERROR("The XML output file is closed. Unable to write to file");
		}
	}
	
	void XmlWriter::Write_start_element_tag(const std::string elementTag) {
		if (outFile.is_open()) {
			for (int i = 0; i < indent; i++) {
				outFile << "\t";
			}
			tempElementTag.resize(openElements + 1);
			tempElementTag[openElements] = elementTag;
			openElements += 1;
			outFile << "<" << elementTag;
		} else {
			PRINT_ERROR("The XML output file is closed. Unable to write to file");
		}
	}

	void XmlWriter::Write_end_element_tag()
	{
		if (outFile.is_open()) {
			outFile << "/>\n";
			tempElementTag.resize(openElements - 1);
			openElements -= 1;
		} else {
			PRINT_ERROR("The XML output file is closed. Unable to write to file");
		}
	}

	void XmlWriter::Write_attribute(const std::string outAttribute)
	{
		if (outFile.is_open()) {
			outFile << " " << outAttribute;
		} else {
			PRINT_ERROR("The XML output file is closed. Unable to write to file");
		}
	}

	void XmlWriter::Write_attribute_string_inline(const std::string attribute_name, const std::string attribute_value)
	{
		if (outFile.is_open()) {
			outFile << " ";
			outFile << attribute_name + "=\"" + attribute_value + "\"";
		} else {
			PRINT_ERROR("The XML output file is closed. Unable to write to file");
		}
	}

	void XmlWriter::Write_string(const std::string outString)
	{
		if (outFile.is_open()) {
			outFile << ">" << outString;
		} else {
			PRINT_ERROR("The XML output file is closed. Unable to write to file");
		}
	}
}