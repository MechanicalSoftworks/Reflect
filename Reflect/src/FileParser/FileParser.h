#pragma once

#include "Core/Core.h"
#include "ReflectStructs.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include <tuple>

NAMESPACE_START

/// <summary>
/// Parse a single file. This should extract all the info like functions and variables.
/// </summary>
class FileParser
{
public:
	FileParser();
	~FileParser();

	void ParseDirectory(const std::string& directory);
	std::ifstream OpenFile(const std::string& filePath);
	void CloseFile(std::ifstream& file);

	const FileParsedData& GetFileParsedData(int index) const { return m_filesParsed.at(index); }

private:
	bool CheckExtension(const std::string& filePath, std::vector<const char*> extensions);

	FileParsedData LoadFile(std::ifstream& file);
	
	void ParseFile(FileParsedData& fileData);
	
	bool ReflectContainerHeader(FileParsedData& fileData, const std::string& keyword, const ReflectType type);
	void ReflectContainer(FileParsedData& fileData);

	int FindEndOfConatiner(const FileParsedData& fileData);

	std::vector<ReflectFlags> ReflectFlags(FileParsedData& fileData);
	char FindNextChar(FileParsedData& fileData, const std::vector<char>& ingoreChars);

	bool RefectCheckForEndOfLine(const FileParsedData& fileData);
	bool ReflectTypeCheck(const std::string& type);
	void ReflectGetFunctionParameters(FileParsedData& fileData);

	std::tuple<std::string, std::string> ReflectTypeAndName(FileParsedData& fileData, const std::vector<char>& endOfLineCharacters);

private:
	std::vector<FileParsedData> m_filesParsed;
};

NAMESPACE_END