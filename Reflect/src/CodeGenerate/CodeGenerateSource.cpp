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
			else
			{
				WriteStaticClass(reflectData, file, addtionalOptions);
			}
		}

		if (addtionalOptions.Namespace.length())
		{
			file << "}" << std::endl;
		}
	}

	void CodeGenerateSource::WriteStaticClass(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		const auto hasAllocator = !Util::ContainsProperty(data.ContainerProps, { "Abstract" });
		const std::string_view allocatorParm = hasAllocator ? "" : "nullptr";

		file << "const Reflect::Class " << data.Name << "::StaticClass = Reflect::Class(" << std::endl
			<< "\t\"" << data.Name << "\", " << std::endl
			<< "\t" << (data.SuperName != "Reflect::IReflect" ? (std::string("&") + data.SuperName + "::StaticClass") : std::string("nullptr")) << "," << std::endl
			<< "\tReflect::ClassAllocator::Create<" << data.Name << ">(" << allocatorParm << ")," << std::endl
			<< "\t" << CodeGenerate::GetMemberProps(data.ContainerProps) << "," << std::endl
			<< CreateMemberInitializerList(data) << "," << std::endl
			<< CreateFunctionInitializerList(data) << std::endl
			<< ");\n\n";
	}

	std::string CodeGenerateSource::CreateMemberInitializerList(const ReflectContainerData& data)
	{
		std::ostringstream oss;

		oss << "\tReflect::Util::make_vector<Reflect::ReflectMemberProp>(\n";

		for (const auto& member : data.Members)
		{
			const auto isLast = &member == &data.Members.back();
			const auto eol = isLast ? "\n" : ",\n";

			std::string readField, writeField;
			if (CodeGenerate::IsSerialised(member))
			{
				readField = "__READ__" + member.Name;
				writeField = "__WRITE__" + member.Name;
			}
			else
			{
				readField = writeField = "nullptr";
			}

			oss << "\t\tReflect::CreateReflectMemberProp<" + member.Type + ">(\"" + member.Name + "\", Reflect::Util::GetTypeName<" + member.Type + ">(), __OFFSETOF__" + member.Name + "(), " + CodeGenerate::GetMemberProps(member.ContainerProps) + ", " + readField + ", " + writeField + ")" + eol;
		}

		oss << "\t)";

		return oss.str();
	}

	std::string CodeGenerateSource::CreateFunctionInitializerList(const ReflectContainerData& data)
	{
		std::ostringstream oss;

		oss << "\tReflect::Util::make_vector<Reflect::ReflectMemberFunction>(\n";

		for (const auto& func : data.Functions)
		{
			const auto isLast = &func == &data.Functions.back();
			const auto eol = isLast ? "\n" : ",\n";

			oss << "\t\tReflect::ReflectMemberFunction(\"" + func.Name + "\", __REFLECT_FUNC__" + func.Name + ")" + eol;
		}

		oss << "\t)";

		return oss.str();
	}

	//void CodeGenerateSource::WriteFunctionBindings(const ReflectContainerData& data, std::ostream& file)
	//{
	//	file << "\t" + data.Name << "* ptr = dynamic_cast<" + data.Name + "*>(this);\n";
	//	file << "\tassert(ptr != nullptr && \"[" + data.Name + ContainerPrefix + "::" + "SetupReflectBindings()] 'ptr' should not be null.\");\n\n";
	//	for (auto& func : data.Functions)
	//	{
	//		// Write to the auto generated file binding a function then setting up a "FuncWrapper" for which we can then call the function from.
	//		file << "\tauto " + func.Name + "Func = std::bind(&" + data.Name + "::" + func.Name + ", ptr);\n";
	//		file << "\tReflect::FuncWrapper " + func.Name + "Wrapper(" + func.Name + "Func);\n";
	//		file << "\tSuperClass::AddFunction(" + func.Name + ", " + func.Name + "Wrapper);\n";
	//	}
	//	file << "\n";
	//}

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
