#include "CodeGenerate/CodeGenerateSource.h"
#include "Instrumentor.h"

namespace Reflect
{
	void CodeGenerateSource::GenerateSource(const FileParsedData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();
		//CodeGenerate::IncludeHeader(data.FileName + ReflectFileGeneratePrefix + ".h", file);
		if (!addtionalOptions.IncludePCHString.empty())
		{
			CodeGenerate::IncludeHeader(addtionalOptions.IncludePCHString, file);
		}
		CodeGenerate::IncludeHeader(data.FileName + "." + data.FileExtension, file);
		CodeGenerate::IncludeHeader("Core/Util.h", file);
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

			std::vector<Reflect::ReflectMemberData> serialiseFields;
			for (const auto& member : reflectData.Members)
			{
				const auto it = std::find_if(member.ContainerProps.begin(), member.ContainerProps.end(), [](const auto& p) { return p == "serialise"; });
				if (it != member.ContainerProps.end())
				{
					serialiseFields.push_back(member);
				}
			}
			WriteDataDictionary(serialiseFields, reflectData, file, addtionalOptions);
			WriteSerialise(serialiseFields, reflectData, file, addtionalOptions);
			WriteUnserialise(serialiseFields, reflectData, file, addtionalOptions);
		}

		if (addtionalOptions.Namespace.length())
		{
			file << "}" << std::endl;
		}
	}

	void CodeGenerateSource::WriteMemberProperties(const ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
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

	void CodeGenerateSource::WriteStaticClass(const ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "const Reflect::Class " << data.Name << "::StaticClass = Reflect::Class(\"" << data.Name << "\", "
			<< "sizeof(" << data.Name << "), alignof(" << data.Name << "), "
			<< (data.SuperName != "Reflect::IReflect" ? (std::string("&") + data.SuperName + "::StaticClass") : std::string("nullptr")) << ", "
			<< data.Members.size() << ", " << (data.Members.size() > 0 ? "__REFLECT_MEMBER_PROPS__" : "nullptr") << ", "
			<< "Reflect::PlacementNew<" << data.Name << ">, Reflect::PlacementDelete<" << data.Name << ">"
			<< ");\n\n";
	}

	void CodeGenerateSource::WriteMemberGet(const ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
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

	void CodeGenerateSource::WriteFunctionGet(const ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
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

	//void CodeGenerateSource::WriteFunctionBindings(const ReflectContainerData& data, std::ofstream& file)
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

	void CodeGenerateSource::WriteDataDictionary(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		if (!serialiseFields.size())
		{
			return;
		}

		file << "const std::array<Reflect::UnserialiseField," << serialiseFields.size() << "> " << data.Name << "::__SERIALISE_FIELDS__ = { \n";
		for (const auto& member : serialiseFields)
		{
			const auto customSerialiser = GetCustomSerialiser(member);
			std::string readField;
			if (customSerialiser.length())
			{
				readField = "Reflect::ReadCustomField<" + customSerialiser + ", " + member.Type + ", __REFLECT__" + member.Name + "()>";
			}
			else
			{
				readField = "Reflect::ReadField <" + member.Type + ", __REFLECT__" + member.Name + "()>";
			}

			file << "\tReflect::UnserialiseField(\"" << member.Name << "\", Reflect::Util::GetTypeName<" + member.Type + ">(), " << readField << "),\n";
		}
		file << "};\n\n";
	}

	void CodeGenerateSource::WriteSerialise(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "void " << data.Name << "::Serialise(Reflect::Serialiser &s, std::ostream &out) const {\n";
		for (const auto& it : serialiseFields)
		{
			const auto customSerialiser = GetCustomSerialiser(it);
			if (customSerialiser.length())
			{
				file << "\tWriteCustomField<" << customSerialiser << ">(s, out, " << it.Name << ");\n";
			}
			else
			{
				file << "\tWriteField(s, out, " << it.Name << ");\n";
			}
		}
		if (data.SuperName.length())
		{
			file << "\tSuperClass::Serialise(s, out);\n";
		}
		file << "}\n\n";
	}

	void CodeGenerateSource::WriteUnserialise(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ofstream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "void " << data.Name << "::Unserialise(Reflect::Unserialiser &u, std::istream &in) {\n";
		if (serialiseFields.size())
		{
			file << "\tu.SetCurrentObject(this);\n";
			file << "\tauto new_it = __SERIALISE_FIELDS__.begin();\n";
			file << "\tconst auto& old_schema = u.GetSchema(\"" << data.Name << "\");\n";	// No need to do a safety check. We've already instantiated the class...means the schema is available.
			file << "\tfor(int i = 0; i < old_schema.fields.size(); i++) {\n";
			file << "\t\tconst auto& field = old_schema.fields[i];\n";
			file << "\t\tnew_it = Reflect::Util::circular_find_if(new_it, __SERIALISE_FIELDS__.begin(), __SERIALISE_FIELDS__.end(), [&field](const auto &f){ return field.name == f.name; });\n";
			file << "\t\tif (new_it == __SERIALISE_FIELDS__.end() || new_it->type != field.type) {\n";
			file << "\t\t\tReflect::SkipField(u, in, field.type);\n";
			file << "\t\t\tcontinue;\n";
			file << "\t\t}\n";
			file << "\t\tnew_it->read(u, in, this);\n";
			file << "\t}\n";
		}
		if (data.SuperName.length())
		{
			file << "\tSuperClass::Unserialise(u, in);\n";
		}
		file << "}\n\n";
	}

	std::string CodeGenerateSource::GetCustomSerialiser(const Reflect::ReflectMemberData& data) const
	{
		const auto it = std::find_if(data.ContainerProps.begin(), data.ContainerProps.end(), [](const auto& p) { return p.find("serialiser=") == 0; });
		if (it != data.ContainerProps.end())
		{
			return it->substr(it->find('=') + 1);
		}

		return "";
	}
}
