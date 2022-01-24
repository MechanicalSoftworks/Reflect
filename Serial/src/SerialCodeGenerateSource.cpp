#include "SerialCodeGenerateSource.h"
#include "Instrumentor.h"

namespace Serial
{
	void SerialCodeGenerateSource::GenerateSource(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();
		//SerialCodeGenerate::IncludeHeader(data.FileName + ReflectFileGeneratePrefix + ".h", file);
		if (!addtionalOptions.IncludePCHString.empty())
		{
			SerialCodeGenerate::IncludeHeader(addtionalOptions.IncludePCHString, file);
		}
		SerialCodeGenerate::IncludeHeader(data.FileName + "." + data.FileExtension, file);
		file << "\n";
		if (addtionalOptions.Namespace.length())
		{
			file << "namespace " << addtionalOptions.Namespace << std::endl;
			file << "{" << std::endl;
		}

		for (auto& reflectData : data.ReflectData)
		{
			WriteMemberProperties(reflectData, file, addtionalOptions);
			WriteStaticClass(reflectData, file, addtionalOptions);
			WriteFunctionGet(reflectData, file, addtionalOptions);
			WriteMemberGet(reflectData, file, addtionalOptions);
		}

		if (addtionalOptions.Namespace.length())
		{
			file << "}" << std::endl;
		}
	}

	void SerialCodeGenerateSource::WriteMemberProperties(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		auto getMemberProps = [](const std::vector<std::string>& flags) -> std::string
		{
			if (flags.size() == 0)
			{
				return "{ }";
			}

			std::string value;
			value += "{";
			for (auto const& flag : flags)
			{
				if (flag != flags.back())
				{
					value += "\"" + flag + "\"" + ", ";
				}
			}
			value += "\"" + flags.back() + "\"" + "}";
			return value;
		};

		if (data.Members.size() > 0)
		{
			file << "Reflect::ReflectMemberProp " + data.Name + "::__REFLECT_MEMBER_PROPS__[" + std::to_string(data.Members.size()) + "] = {\n";
			for (const auto& member : data.Members)
			{
				file << "\tReflect::ReflectMemberProp(\"" + member.Name + "\", Reflect::Util::GetTypeName<" + member.Type + ">(), __REFLECT__" + member.Name + "(), " + getMemberProps(member.ContainerProps) + "),\n";
			}
			file << "};\n\n";
		}
	}

	void SerialCodeGenerateSource::WriteStaticClass(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "const Reflect::Class " << data.Name << "::StaticClass = Reflect::Class(\"" << data.Name << "\", "
			<< "sizeof(" << data.Name << "), alignof(" << data.Name << "), "
			<< (data.SuperName != "Reflect::IReflect" ? (std::string("&") + data.SuperName + "::StaticClass") : std::string("nullptr")) << ", "
			<< data.Members.size() << ", " << (data.Members.size() > 0 ? "__REFLECT_MEMBER_PROPS__" : "nullptr") << ", "
			<< "Reflect::PlacementNew<" << data.Name << ">, Reflect::PlacementDelete<" << data.Name << ">"
			<< ");\n\n";
	}

	void SerialCodeGenerateSource::WriteMemberGet(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "Reflect::ReflectMember " + data.Name + "::GetMember(const std::string_view& memberName)\n{\n";
		if (data.Members.size() > 0)
		{
			file << "\tfor(const auto& member : __REFLECT_MEMBER_PROPS__)\n\t{\n";
			file << "\t\tif(memberName == member.Name)\n";
			file << "\t\t{\n";
			file << "\t\t\t//CheckFlags\n";
			file << "\t\t\treturn Reflect::ReflectMember(member.Name, member.Type, ((char*)this) + member.Offset);\n";
			file << "\t\t}\n";
			file << "\t}\n";
		}
		file << "\treturn SuperClass::GetMember(memberName);\n";
		file << "}\n\n";

		file << "std::vector<Reflect::ReflectMember> " + data.Name + "::GetMembers(std::vector<std::string> const& flags)\n{\n";
		file << "\tstd::vector<Reflect::ReflectMember> members = SuperClass::GetMembers(flags);\n";
		if (data.Members.size() > 0)
		{
			file << "\tfor(auto& member : __REFLECT_MEMBER_PROPS__)\n\t{\n";
			file << "\t\tif(member.ContainsProperty(flags))\n";
			file << "\t\t{\n";
			file << "\t\t\tmembers.push_back(Reflect::ReflectMember(member.Name, member.Type, ((char*)this) + member.Offset));\n";
			file << "\t\t}\n";
			file << "\t}\n";
		}
		file << "\treturn members;\n";
		file << "}\n\n";
	}

	void SerialCodeGenerateSource::WriteFunctionGet(const Reflect::ReflectContainerData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "Reflect::ReflectFunction " + data.Name + "::GetFunction(const std::string_view &functionName)\n{\n";
		for (const auto& func : data.Functions)
		{
			file << "\tif(functionName == \"" + func.Name + "\")\n";
			file << "\t{\n";
			file << "\t\treturn Reflect::ReflectFunction(this, " + data.Name + "::__REFLECT_FUNC__" + func.Name + ");\n";
			file << "\t}\n";
		}
		file << "\treturn SuperClass::GetFunction(functionName);\n";
		file << "}\n\n";
	}

	//void SerialCodeGenerateSource::WriteFunctionBindings(const Reflect::ReflectContainerData& data, std::ofstream& file)
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
}
