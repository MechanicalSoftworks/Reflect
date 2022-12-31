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
		std::string RelativeFilePath;
		std::string ExportMacro;
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

		static std::string GetMemberProps(const std::vector<std::string>& flags);

		static bool IsSerialised(const Reflect::ReflectMemberData& data);

	private:
		static void WriteIfDifferent(const std::string& filePath, const std::string& str);
	};
}