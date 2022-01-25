#pragma once

#include "Core/Core.h"
#include "ReflectStructs.h"
#include "CodeGenerate/CodeGenerate.h"

namespace Reflect
{
	class CodeGenerateSource
	{
	public:
		CodeGenerateSource() { }
		~CodeGenerateSource() { }

		void GenerateSource(const FileParsedData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

	private:
		void WriteMemberProperties(const ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteStaticClass(const ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

		void WriteMemberGet(const ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteFunctionGet(const ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

		void WriteDataDictionary(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteSerialise(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteUnserialise(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
	};
}
