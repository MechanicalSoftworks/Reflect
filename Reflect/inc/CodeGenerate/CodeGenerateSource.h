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
		std::string CreateMemberInitializerList(const ReflectContainerData& data);
		std::string CreateFunctionInitializerList(const ReflectContainerData& data);

		void WriteStaticEnum(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions);
	};
}
