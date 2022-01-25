#include "Serial.h"
#include "SerialCodeGenerateHeader.h"
#include "SerialCodeGenerate.h"
#include "Instrumentor.h"
#include <assert.h>

namespace Serial
{
	std::string GetCurrentFileID(const std::string& fileName)
	{
		return  fileName + "_Source_h";
	}

#define WRITE_CURRENT_FILE_ID(FileName) file << "#define " + GetCurrentFileID(FileName)
#define WRITE_CLOSE() file << "\n\n"

#define WRITE_PUBLIC() file << "public:\\\n"
#define WRITE_PRIVATE() file << "private:\\\n"

	void SerialCodeGenerateHeader::GenerateHeader(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();

		file << " // This file is auto generated please don't modify.\n";

		SerialCodeGenerate::IncludeHeader("ReflectStructs.h", file);
		SerialCodeGenerate::IncludeHeader("Core/Util.h", file);

		file << "\n";
		file << "#ifdef " + data.FileName + SerialFileHeaderGuard + "_h\n";
		file << "#error \"" + data.FileName + SerialFileHeaderGuard + ".h" + " already included, missing 'pragma once' in " + data.FileName + ".h\"\n";
		file << "#endif " + data.FileName + SerialFileHeaderGuard + "_h\n";
		file << "#define " + data.FileName + SerialFileHeaderGuard + "_h\n\n";

		file << "\n";
		file << "namespace tera { class Serialise; }\n";
		file << "namespace tera { class Unserialise; }\n";

		WriteMacros(data, file, addtionalOptions);
	}

	void SerialCodeGenerateHeader::WriteMacros(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		for (const auto& reflectData : data.ReflectData)
		{
			const std::string CurrentFileId = GetCurrentFileID(data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine + 1);

			std::vector<Reflect::ReflectMemberData> serialiseFields;
			for (const auto& member : reflectData.Members)
			{
				const auto it = std::find_if(member.ContainerProps.begin(), member.ContainerProps.end(), [](const auto& p) { return p == "serialise"; });
				if (it != member.ContainerProps.end())
				{
					serialiseFields.push_back(member);
				}
			}

			WriteDataDictionary(serialiseFields, reflectData, file, CurrentFileId, addtionalOptions);
			WriteMethods(serialiseFields, reflectData, file, CurrentFileId, addtionalOptions);

			WRITE_CURRENT_FILE_ID(data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine + 1) + "_SERIAL_GENERATED_BODY \\\n";
			file << CurrentFileId + "_DATA_DICTIONARY \\\n";
			file << CurrentFileId + "_METHODS \\\n";

			WRITE_CLOSE();
		}

		file << "#undef CURRENT_FILE_ID\n";
		file << "#define CURRENT_FILE_ID " + GetCurrentFileID(data.FileName) + "\n";
	}

	void SerialCodeGenerateHeader::WriteDataDictionary(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_DATA_DICTIONARY \\\n";
		WRITE_PUBLIC();
		if (serialiseFields.size())
		{
			file << "static constexpr std::vector<Serial::UnserialiseField> UnserialiseFields{ \\\n";
			for (const auto& member : serialiseFields)
			{
				file << "\t{ \"" << member.Name << "\", Serialise::ReadFieldType<" << data.Name << ", __REFLECT__" << member.Name << "()> }, \\\n";
			}
			file << "};\n";
		}
		WRITE_CLOSE();
	}

	void SerialCodeGenerateHeader::WriteMethods(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_METHODS \\\n";
		WRITE_PUBLIC();

		// Always write - sometimes we might need to passthrough a class.
		file << "virtual void serialise(tera::Serialise &s, std::ostream &out);\\\n";
		file << "virtual void unserialise(tera::Unserialise &s, std::istream &in);\\\n";

		WRITE_CLOSE();
	}
}