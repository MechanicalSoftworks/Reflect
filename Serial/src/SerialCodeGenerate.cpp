#include "Serial.h"
#include "SerialCodeGenerate.h"
#include "SerialCodeGenerateHeader.h"
#include "SerialCodeGenerateSource.h"
#include "Instrumentor.h"
#include "ReflectStructs.h"
#include <assert.h>
#include <filesystem>

namespace Serial
{
	constexpr const char* ContainerPrefix = "ReflectObject";

	SerialCodeGenerate::SerialCodeGenerate()
	{ }

	SerialCodeGenerate::~SerialCodeGenerate()
	{ }

	void SerialCodeGenerate::Generate(const Reflect::FileParsedData& data, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();

		SerialCodeGenerateHeader header;
		SerialCodeGenerateSource source;

		std::ofstream file = OpenFile(data.FilePath + "/" + data.FileName + SerialFileGeneratePrefix + ".h");
		header.GenerateHeader(data, file, addtionalOptions);
		CloseFile(file);

		file = OpenFile(addtionalOptions.OutputCPPDir + "/" + data.FileName + SerialFileGeneratePrefix + ".cpp");
		source.GenerateSource(data, file, addtionalOptions);
		CloseFile(file);
	}

	std::ofstream SerialCodeGenerate::OpenFile(const std::string& filePath)
	{
		std::ofstream file;
		file.open(filePath, std::ios::trunc);
		assert(file.is_open() && "[SerialCodeGenerate::OpenFile] File could not be created.");
		return file;
	}

	void SerialCodeGenerate::CloseFile(std::ofstream& file)
	{
		if (file.is_open())
		{
			file.close();
		}
	}

	void SerialCodeGenerate::IncludeHeader(const std::string& headerToInclude, std::ofstream& file, bool windowsInclude)
	{
		if (windowsInclude)
		{
			file << "#include <" + headerToInclude + ">\n";
		}
		else
		{
			file << "#include \"" + headerToInclude + "\"\n";
		}
	}
}
