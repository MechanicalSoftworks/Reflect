#pragma once

#include "Core/Core.h"
#include "ReflectStructs.h"
#include "SerialCodeGenerate.h"
#include<fstream>

namespace Serial
{
	class SerialCodeGenerateHeader
	{
	public:
		SerialCodeGenerateHeader() { }
		~SerialCodeGenerateHeader() { }

		void GenerateHeader(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);

	private:
		void WriteMacros(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteDataDictionary(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteMethods(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
	};
}
