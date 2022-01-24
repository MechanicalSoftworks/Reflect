#pragma once

#include "Core/Core.h"
#include "ReflectStructs.h"
#include<fstream>

namespace Serial
{
	struct SerialCodeGenerateAddtionalOptions
	{
		std::string IncludePCHString;
		std::string OutputCPPDir;
		std::string Namespace;
	};

	/// <summary>
	/// Generate a new file and place the reflect code there.
	/// </summary>
	class SerialCodeGenerate
	{
	public:
		SerialCodeGenerate();
		~SerialCodeGenerate();

		void Reflect(const Reflect::FileParsedData& data, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		static void IncludeHeader(const std::string& headerToInclude, std::ofstream& file, bool windowsInclude = false);

	private:
		std::ofstream OpenFile(const std::string& filePath);
		void CloseFile(std::ofstream& file);
	};
}