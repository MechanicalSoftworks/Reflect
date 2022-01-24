#pragma once

#include "Core/Core.h"
#include "ReflectStructs.h"
#include "SerialCodeGenerate.h"

namespace Serial
{
	class SerialCodeGenerateSource
	{
	public:
		SerialCodeGenerateSource() { }
		~SerialCodeGenerateSource() { }

		void GenerateSource(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);

	private:
		void WriteMemberProperties(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteStaticClass(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);

		void WriteMemberGet(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteFunctionGet(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
	};
}
