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

		void GenerateSource(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

	private:
		using SerialiseFields = std::vector<Reflect::ReflectMemberData>;

		void WriteMemberProperties(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteStaticClass(const ReflectContainerData& data, const SerialiseFields& serialiseFields, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

		void WriteMemberGet(const ReflectContainerData& data, const SerialiseFields& serialiseFields, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteFunctionGet(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

		void WriteDataDictionary(const SerialiseFields& serialiseFields, const Reflect::ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteSerialise(const SerialiseFields& serialiseFields, const Reflect::ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteUnserialise(const SerialiseFields& serialiseFields, const Reflect::ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

		void WriteEnum(const Reflect::ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		
		std::string GetCustomSerialiser(const Reflect::ReflectMemberData&data) const;
		std::string GetMemberProps(const std::vector<std::string>& flags) const;
	};
}
