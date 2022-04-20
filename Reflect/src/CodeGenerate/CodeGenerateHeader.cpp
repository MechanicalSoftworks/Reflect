#include "CodeGenerate/CodeGenerateHeader.h"
#include "CodeGenerate/CodeGenerate.h"
#include "Instrumentor.h"
#include <assert.h>

namespace Reflect
{
	std::string GetCurrentFileID(const CodeGenerateAddtionalOptions& addtionalOptions, const std::string& fileName)
	{
		auto relative = addtionalOptions.RelativeFilePath;
		std::replace(relative.begin(), relative.end(), '/',  '_');
		std::replace(relative.begin(), relative.end(), '\\', '_');

		return relative + "_" + fileName + "_Source_h";
	}

#define WRITE_CURRENT_FILE_ID(FileName) file << "#define " + GetCurrentFileID(addtionalOptions, FileName)
#define WRITE_CLOSE() file << "\n\n"

#define WRITE_PUBLIC() file << "public:\\\n"
#define WRITE_PRIVATE() file << "private:\\\n"

	void CodeGenerateHeader::GenerateHeader(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();

		file << " // This file is auto generated please don't modify.\n";

		CodeGenerate::IncludeHeader("ReflectStructs.h", file);
		CodeGenerate::IncludeHeader("Core/Core.h", file);
		CodeGenerate::IncludeHeader("Core/Serial.h", file);
		CodeGenerate::IncludeHeader("Core/Util.h", file);
		CodeGenerate::IncludeHeader("array", file, true);

		std::string reflectGuard = addtionalOptions.RelativeFilePath + data.FileName + ReflectFileHeaderGuard;
		std::replace(reflectGuard.begin(), reflectGuard.end(), '/',  '_');
		std::replace(reflectGuard.begin(), reflectGuard.end(), '\\', '_');
		std::replace(reflectGuard.begin(), reflectGuard.end(), '.', '_');
		std::replace(reflectGuard.begin(), reflectGuard.end(), ':', '_');
		std::replace(reflectGuard.begin(), reflectGuard.end(), '-', '_');

		file << "\n";
		file << "#ifdef " + reflectGuard + "_h\n";
		file << "#error \"" + reflectGuard + ".h" + " already included, missing 'pragma once' in " + data.FileName + ".h\"\n";
		file << "#endif //" + reflectGuard + "_h\n";
		file << "#define " + reflectGuard + "_h\n\n";

		WriteMacros(data, file, addtionalOptions);
		WriteEnums(data, file, addtionalOptions);
	}

	void CodeGenerateHeader::WriteMacros(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		for (const auto& reflectData : data.ReflectData)
		{
			if (reflectData.ReflectType == ReflectType::Enum)
			{
				continue;
			}

			const std::string CurrentFileId = GetCurrentFileID(addtionalOptions, data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine);

			WriteStaticClass(reflectData, file, CurrentFileId, addtionalOptions);
			WriteMemberProperties(reflectData, file, CurrentFileId, addtionalOptions);
			WriteFunctions(reflectData, file, CurrentFileId, addtionalOptions);
			WriteFunctionGet(reflectData, file, CurrentFileId, addtionalOptions);
			WriteMemberPropertiesOffsets(reflectData, file, CurrentFileId, addtionalOptions);
			WriteMemberGet(reflectData, file, CurrentFileId, addtionalOptions);

			std::vector<Reflect::ReflectMemberData> serialiseFields;
			for (const auto& member : reflectData.Members)
			{
				const auto it = std::find_if(member.ContainerProps.begin(), member.ContainerProps.end(), [](const auto& p) { return p == "Serialise"; });
				if (it != member.ContainerProps.end())
				{
					serialiseFields.push_back(member);
				}
			}
			WriteDataDictionary(serialiseFields, reflectData, file, CurrentFileId, addtionalOptions);
			WriteSerialiseMethods(serialiseFields, reflectData, file, CurrentFileId, addtionalOptions);

			WRITE_CURRENT_FILE_ID(data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine) + "_GENERATED_BODY \\\n";
			file << CurrentFileId + "_STATIC_CLASS \\\n";
			file << CurrentFileId + "_PROPERTIES \\\n";
			file << CurrentFileId + "_FUNCTION_DECLARE \\\n";
			file << CurrentFileId + "_FUNCTION_GET \\\n";
			file << CurrentFileId + "_PROPERTIES_OFFSET \\\n";
			file << CurrentFileId + "_PROPERTIES_GET \\\n";
			file << CurrentFileId + "_DATA_DICTIONARY \\\n";
			file << CurrentFileId + "_SERIALISE_METHODS \\\n";

			WRITE_CLOSE();
		}

		file << "#undef CURRENT_FILE_ID\n";
		file << "#define CURRENT_FILE_ID " + GetCurrentFileID(addtionalOptions, data.FileName) + "\n";
	}

	void CodeGenerateHeader::WriteStaticClass(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_STATIC_CLASS \\\n";
		WRITE_PUBLIC();
		if (data.SuperName.length())
			file << "\ttypedef " + data.SuperName + " SuperClass;\\\n";
		file << "\tstatic const Reflect::Class StaticClass;\\\n";
		file << "\tstatic void __PlacementNew(" << data.Name << "* obj, const Reflect::Initialiser& init) { new(obj) " << data.Name << "(init); }\\\n";
		file << "\tstatic void __PlacementDelete(" << data.Name << "* obj) { obj->~" << data.Name << "(); }\\\n";
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteMemberProperties(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_PROPERTIES \\\n";
		WRITE_PRIVATE();
		file << "\tstatic Reflect::ReflectMemberProp __REFLECT_MEMBER_PROPS__[" + std::to_string(data.Members.size()) + "];\\\n";
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteMemberPropertiesOffsets(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_PROPERTIES_OFFSET \\\n";
		WRITE_PRIVATE();
		for (const auto& member : data.Members)
		{
			file << "\tstatic constexpr int __REFLECT__" + member.Name + "() { return offsetof(" + data.Name + ", " + member.Name + "); }; \\\n";
		}
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteMemberGet(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_PROPERTIES_GET \\\n";
		WRITE_PUBLIC();
		file << "virtual Reflect::ReflectMember GetMember(const std::string_view& memberName) override;\\\n";
		file << "virtual std::vector<Reflect::ReflectMember> GetMembers(std::vector<std::string> const& flags) override;\\\n";
		for (const auto& member : data.Members)
		{
			file << "static constexpr const char* " << member.Name << "_name = \"" << member.Name << "\";\\\n";
		}
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteFunctions(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		//TODO: Pass in parameters in someway. Prob need to use templates.

		auto populateArgs = [](const std::vector<ReflectTypeNameData>& args) -> std::string
		{
			std::string returnValue;
			for (const auto& arg : args)
			{
				returnValue += arg.Name + "Arg";
				if (arg != args.back())
				{
					returnValue += ", ";
				}
			}
			return returnValue;
		};

		auto castToType = [](const Reflect::ReflectTypeNameData& arg) -> std::string
		{
			if (arg.ReflectMemberType == Reflect::ReflectMemberType::Pointer)
			{
				return "static_cast<" + arg.Type + ">";
			}
			else if (arg.ReflectMemberType == Reflect::ReflectMemberType::Reference)
			{
				return "*static_cast<" + arg.Type.substr(0, arg.Type.length() - 1) + "*>";
			}
			else
			{
				return "*static_cast<" + arg.Type + "*>";
			}
		};

		file << "#define " + currentFileId + "_FUNCTION_DECLARE \\\n";
		WRITE_PRIVATE();
		for (const auto& func : data.Functions)
		{
			file << "\tstatic Reflect::ReflectReturnCode __REFLECT_FUNC__" + func.Name + "(void* objectPtr, void* returnValuePtr, Reflect::FunctionPtrArgs& functionArgs)\\\n";
			file << "\t{\\\n";
			int functionArgIndex = 0;
			for (const auto& arg : func.Parameters)
			{
				file << "\t\t" + arg.Type + " " + arg.Name + "Arg = " + castToType(arg) + "(functionArgs.GetArg(" + std::to_string(functionArgIndex++) + "));\\\n";
			}
			file << "\t\t" + data.Name + "* ptr = static_cast<" + data.Name + "*>(objectPtr);\\\n";
			if (func.Type != "void")
			{
				file << "\t\t*((" + func.Type + "*)returnValuePtr) = ptr->" + func.Name + "(" + populateArgs(func.Parameters) + ");\\\n";
				// TODO: (01/04/21) Check this cast. If it failed return ReflectFuncReturnCode::CAST_FAILED.
			}
			else
			{
				file << "\t\tptr->" + func.Name + "();\\\n";
			}
			file << "\t\treturn Reflect::ReflectReturnCode::SUCCESS;\\\n";
			file << "\t}\\\n";
		}
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteFunctionGet(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_FUNCTION_GET \\\n";
		WRITE_PUBLIC();
		file << "\tvirtual Reflect::ReflectFunction GetFunction(const std::string_view &functionName) override;\\\n";
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteDataDictionary(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_DATA_DICTIONARY \\\n";
		WRITE_PUBLIC();
		if (serialiseFields.size())
		{
			file << "static const std::array<Reflect::UnserialiseField," << serialiseFields.size() << "> __SERIALISE_FIELDS__;\\\n";
		}
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteSerialiseMethods(const std::vector<Reflect::ReflectMemberData>& serialiseFields, const Reflect::ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_SERIALISE_METHODS \\\n";
		WRITE_PUBLIC();

		// Always write - sometimes we might need to passthrough a class.
		file << "virtual void Serialise(Reflect::Serialiser &s, std::ostream &out) const override;\\\n";
		file << "virtual void Unserialise(Reflect::Unserialiser &u, std::istream &in) override;\\\n";

		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteEnums(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "\n\n";

		for (const auto& reflectData : data.ReflectData)
		{
			if (reflectData.ReflectType != ReflectType::Enum)
			{
				continue;
			}

			if (addtionalOptions.Namespace.length())
			{
				file << "namespace " << addtionalOptions.Namespace << "{\n";
			}

			file << "enum class " << reflectData.Type << " : " << reflectData.SuperName << ";\n";

			file << "inline const char* EnumToString(" << reflectData.Type << " x) {\n";
			file << "\tswitch((" << reflectData.SuperName << ")x) {\n";
			for (const auto& c : reflectData.Constants)
			{
				file << "\t\tcase " << c.Value << ": return \"" << c.Name << "\";\n";
			}
			file << "\t}\n";
			file << "\treturn nullptr;\n";
			file << "}\n\n";

			file << "inline bool StringToEnum(const std::string_view &str, " << reflectData.Type << "& x) {\n";
			for (const auto& c : reflectData.Constants)
			{
				file << "\tif (str == " << "\"" << c.Name << "\") { x = " << reflectData.Type << "(" << c.Value << "); return true; }\n";
			}
			file << "\treturn false;\n";
			file << "}\n\n";

			const auto valueVector = "std::vector<std::pair<std::string, " + reflectData.Type + ">>";

			file << "template<typename T> inline const std::vector<std::pair<std::string, T>>& EnumValues();\n";

			file << "template<> inline const " << valueVector << "& EnumValues() { \n";
			file << "\tstatic " << valueVector << " values{\n";
			for (const auto& c : reflectData.Constants)
			{
				file << "\t\tstd::pair<std::string, " << reflectData.Type << ">(\"" << c.Name << "\", " << reflectData.Type << "(" << c.Value << ")),\n";
			}
			file << "\t};\n";
			file << "\treturn values;\n";
			file << "}\n\n";

			if (addtionalOptions.Namespace.length())
			{
				file << "}\n";
			}
		}
	}
}