#pragma once

#include "Core/Core.h"
#include "ReflectStructs.h"
#include "CodeGenerate/CodeGenerate.h"
#include<fstream>

namespace Reflect
{
	class CodeGenerateHeader
	{
	public:
		CodeGenerateHeader() { }
		~CodeGenerateHeader() { }

		void GenerateHeader(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void GenerateStaticHeader(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

	private:
		void WriteMacros(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteStatic(const Reflect::ReflectContainerData& reflectData, const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

		void WriteClassMacros(const Reflect::ReflectContainerData& reflectData, const FileParsedData& data, std::ostream& file, const std::string& CurrentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteStaticClass(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteMemberProperties(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteMemberPropertiesOffsets(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteFunctions(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);

		std::string CreateMemberInitializerList(const ReflectContainerData& data);
		std::string CreateFunctionInitializerList(const ReflectContainerData& data);
		std::string CreateInterfaceList(const ReflectContainerData& data);

		void WriteEnumMacros(const Reflect::ReflectContainerData& reflectData, const FileParsedData& data, std::ostream& file, const std::string& CurrentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteStaticEnum(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteEnumOperators(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteEnumValues(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteEnumMembers(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteEnumMethods(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions);
	};
}
