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
		void WriteStaticClass(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteMemberProperties(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteMemberPropertiesOffsets(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteMemberGet(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions);

		void WriteFunctions(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteFunctionGet(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
	};
}
