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
		void WriteSerialise(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
		void WriteUnserialise(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions);
	};
}
