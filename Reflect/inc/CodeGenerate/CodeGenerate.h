#pragma once

#include "Core/Core.h"
#include "ReflectStructs.h"
#include<fstream>

namespace Reflect
{
	struct CodeGenerateAddtionalOptions
	{
		std::string IncludePCHString;
		std::string OutputCPPDir;
		std::string Namespace;
	};

	/// <summary>
	/// Generate a new file and place the reflect code there.
	/// </summary>
	class CodeGenerate
	{
	public:
		REFLECT_DLL CodeGenerate();
		REFLECT_DLL ~CodeGenerate();

		REFLECT_DLL void Reflect(const FileParsedData& data, const CodeGenerateAddtionalOptions& addtionalOptions);
		static void IncludeHeader(const std::string& headerToInclude, std::ostream& file, bool windowsInclude = false);

	private:
		static void WriteIfDifferent(const std::string& filePath, const std::string& str);
	};
}