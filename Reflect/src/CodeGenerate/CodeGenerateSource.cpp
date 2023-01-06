#include "CodeGenerate/CodeGenerateSource.h"
#include "Instrumentor.h"

namespace Reflect
{
	void CodeGenerateSource::GenerateSource(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();
		//CodeGenerate::IncludeHeader(data.FileName + ReflectFileGeneratePrefix + ".h", file);
		if (!addtionalOptions.IncludePCHString.empty())
		{
			CodeGenerate::IncludeHeader(addtionalOptions.IncludePCHString, file);
		}
		const std::string prefix = addtionalOptions.RelativeFilePath.length()
			? addtionalOptions.RelativeFilePath + "/"
			: "";
		CodeGenerate::IncludeHeader(prefix + data.FileName + "." + data.FileExtension, file);
		CodeGenerate::IncludeHeader("Core/Util.h", file);
		file << "\n";
		if (addtionalOptions.Namespace.length())
		{
			file << "namespace " << addtionalOptions.Namespace << std::endl;
			file << "{" << std::endl;
		}

		for (auto& reflectData : data.ReflectData)
		{
			if (reflectData.ReflectType == ReflectType::Enum)
			{
				WriteStaticEnum(reflectData, file, addtionalOptions);
			}
		}

		if (addtionalOptions.Namespace.length())
		{
			file << "}" << std::endl;
		}
	}

	void CodeGenerateSource::WriteStaticEnum(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		if (!Util::ContainsProperty(data.ContainerProps, { "ValueType" }))
		{
			file << "#error \"Enum " << data.Name << " is missing ValueType\"\n";
			return;
		}

		std::string_view valueType;
		Util::TryGetPropertyValue(data.ContainerProps, "ValueType", valueType);

		file << "const Reflect::Enum " << data.Name << "::StaticEnum = Reflect::Enum(\"" << data.Name << "\", Reflect::Util::GetTypeName<" << data.Name << "::ValueType>(),\n";
		file << "\t" << CodeGenerate::GetMemberProps(data.ContainerProps) << ", \n";

		file << "\t{\n";
		for (const auto& c : data.Constants)
		{
			std::string_view displayLabel;
			if (!Util::TryGetPropertyValue(c.Flags, "DisplayLabel", displayLabel))
			{
				displayLabel = c.Name;
			}

			file << "\t\tReflect::EnumConstant(\"" << c.Name << "\", " << c.Value << ", \"" << displayLabel << "\", " << CodeGenerate::GetMemberProps(c.Flags) << "),\n";
		}
		file << "\t},\n";

		file << "\t[](const void* ptr) -> Reflect::Enum::ConstantType { return ((" << data.Name << "*)ptr)->Value; },\n";
		file << "\t[](void* ptr, Reflect::Enum::ConstantType value) -> void { ((" << data.Name << "*)ptr)->Value = (" << data.Name << "::Values)value; } \n";

		file << ");\n\n";
	}
}
