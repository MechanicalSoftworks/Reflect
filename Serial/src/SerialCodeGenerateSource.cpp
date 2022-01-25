#include "SerialCodeGenerateSource.h"
#include "Instrumentor.h"

namespace Serial
{
	void SerialCodeGenerateSource::GenerateSource(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();
		//SerialCodeGenerate::IncludeHeader(data.FileName + ReflectFileGeneratePrefix + ".h", file);
		if (!addtionalOptions.IncludePCHString.empty())
		{
			SerialCodeGenerate::IncludeHeader(addtionalOptions.IncludePCHString, file);
		}
		SerialCodeGenerate::IncludeHeader("framework/serialise.hpp", file);
		SerialCodeGenerate::IncludeHeader(data.FileName + "." + data.FileExtension, file);
		file << "\n";
		if (addtionalOptions.Namespace.length())
		{
			file << "namespace " << addtionalOptions.Namespace << std::endl;
			file << "{" << std::endl;
		}

		for (auto& reflectData : data.ReflectData)
		{
			WriteSerialise(reflectData, file, addtionalOptions);
			WriteUnserialise(reflectData, file, addtionalOptions);
		}

		if (addtionalOptions.Namespace.length())
		{
			file << "}" << std::endl;
		}
	}

	void SerialCodeGenerateSource::WriteSerialise(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "void " << data.Name << "::serialise(tera::Serialise &s, std::ostream &out) {\n";
		file << "	\n";
		file << "}\n\n";
	}

	void SerialCodeGenerateSource::WriteUnserialise(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "void " << data.Name << "::unserialise(tera::Unserialise &s, std::ostream &out) {\n";
		file << "	\n";
		file << "}\n\n";
	}
}
