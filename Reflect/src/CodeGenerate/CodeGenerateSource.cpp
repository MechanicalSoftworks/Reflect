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
				WriteFunctionGet(reflectData, file, addtionalOptions);
				WriteMemberGet(reflectData, file, addtionalOptions);
			}
		}

		if (addtionalOptions.Namespace.length())
		{
			file << "}" << std::endl;
		}
	}

	void CodeGenerateSource::WriteStaticClass(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "const Reflect::Class " << data.Name << "::StaticClass = Reflect::Class(\"" << data.Name << "\", "
			<< (data.SuperName != "Reflect::IReflect" ? (std::string("&") + data.SuperName + "::StaticClass") : std::string("nullptr")) << ", "
			<< CodeGenerate::GetMemberProps(data.ContainerProps) << ", "
			<< data.Members.size() << ", " << (data.Members.size() > 0 ? "__REFLECT_MEMBER_PROPS__.data()" : "nullptr") << ", "
			<< "Reflect::ClassAllocator::Create<" << data.Name << ">()"
			<< ");\n\n";
	}

	void CodeGenerateSource::WriteMemberGet(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "Reflect::ReflectMember " + data.Name + "::GetMember(const std::string_view& memberName) const\n{\n";
		if (data.Members.size() > 0)
		{
			file << "\tfor(const auto& member : __REFLECT_MEMBER_PROPS__)\n\t{\n";
			file << "\t\tif(memberName == member.Name)\n";
			file << "\t\t{\n";
			file << "\t\t\treturn Reflect::ReflectMember(&member, ((char*)this) + member.Offset);\n";
			file << "\t\t}\n";
			file << "\t}\n";
		}
		file << "\treturn SuperClass::GetMember(memberName);\n";
		file << "}\n\n";

		file << "std::vector<Reflect::ReflectMember> " + data.Name + "::GetMembers(std::vector<std::string> const& flags) const\n{\n";
		file << "\tauto members = SuperClass::GetMembers(flags);\n";
		if (data.Members.size() > 0)
		{
			file << "\tfor(auto& member : __REFLECT_MEMBER_PROPS__)\n\t{\n";
			file << "\t\tif(member.ContainsProperty(flags))\n";
			file << "\t\t{\n";
			file << "\t\t\tmembers.push_back(Reflect::ReflectMember(&member, ((char*)this) + member.Offset));\n";
			file << "\t\t}\n";
			file << "\t}\n";
		}
		file << "\treturn members;\n";
		file << "}\n\n";

		file << "std::vector<Reflect::ReflectMember> " + data.Name + "::GetMembers() const\n{\n";
		file << "\tauto members = SuperClass::GetMembers();\n";
		if (data.Members.size() > 0)
		{
			file << "\tmembers.reserve(members.size() + __REFLECT_MEMBER_PROPS__.size());\n";
			file << "\tfor(auto& member : __REFLECT_MEMBER_PROPS__)\n\t{\n";
			file << "\t\tmembers.push_back(Reflect::ReflectMember(&member, ((char*)this) + member.Offset));\n";
			file << "\t}\n";
		}
		file << "\treturn members;\n";
		file << "}\n\n";
	}

	void CodeGenerateSource::WriteFunctionGet(const ReflectContainerData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "Reflect::ReflectFunction " + data.Name + "::GetFunction(const std::string_view &functionName) const\n{\n";
		for (const auto& func : data.Functions)
		{
			file << "\tif(functionName == \"" + func.Name + "\")\n";
			file << "\t{\n";
			file << "\t\treturn Reflect::ReflectFunction(const_cast<" + data.Name + "*>(this), " + data.Name + "::__REFLECT_FUNC__" + func.Name + ");\n";
			file << "\t}\n";
		}
		file << "\treturn SuperClass::GetFunction(functionName);\n";
		file << "}\n\n";
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
		Util::GetPropertyValue(data.ContainerProps, "ValueType", valueType);

		file << "const Reflect::Enum " << data.Name << "::StaticEnum = Reflect::Enum(\"" << data.Name << "\", \"" << valueType << "\",\n";
		file << "\t" << CodeGenerate::GetMemberProps(data.ContainerProps) << ", \n";

		file << "\t{\n";
		for (const auto& c : data.Constants)
		{
			std::string_view displayLabel;
			if (!Util::GetPropertyValue(c.Flags, "DisplayLabel", displayLabel))
			{
				displayLabel = c.Name;
			}

			file << "\t\tReflect::EnumConstant(\"" << c.Name << "\", " << c.Value << ", \"" << displayLabel << "\", " << CodeGenerate::GetMemberProps(c.Flags) << "),\n";
		}
		file << "\t},\n";

		file << "\t{\n";
		for (const auto& c : data.Constants)
		{
			file << "\t\t{ \"" << c.Name << "\", Definition(" << c.Value << ") },\n";
		}
		file << "\t},\n";

		file << "\t{\n";
		for (const auto& c : data.Constants)
		{
			file << "\t\t{ Definition(" << c.Value << "), \"" << c.Name << "\" },\n";
		}
		file << "\t},\n";

		file << "\t[](const void* ptr) -> Reflect::Enum::ConstantType { return ((" << data.Name << "*)ptr)->Value; },\n";
		file << "\t[](void* ptr, Reflect::Enum::ConstantType value) -> void { ((" << data.Name << "*)ptr)->Value = (" << data.Name << "::ValueType)value; } \n";

		file << ");\n\n";
	}
}
