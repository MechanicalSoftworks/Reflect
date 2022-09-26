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
		void WriteStaticClass(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

		void WriteMemberGet(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
		void WriteFunctionGet(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);

		void WriteEnum(const Reflect::ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
	};
}
