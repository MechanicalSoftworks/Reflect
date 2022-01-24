#include "Serial.h"
#include "SerialCodeGenerateHeader.h"
#include "SerialCodeGenerate.h"
#include "Instrumentor.h"
#include <assert.h>

namespace Serial
{
	std::string GetCurrentFileID(const std::string& fileName)
	{
		return  fileName + "_Source_h";
	}

#define WRITE_CURRENT_FILE_ID(FileName) file << "#define " + GetCurrentFileID(FileName)
#define WRITE_CLOSE() file << "\n\n"

#define WRITE_PUBLIC() file << "public:\\\n"
#define WRITE_PRIVATE() file << "private:\\\n"

	void SerialCodeGenerateHeader::GenerateHeader(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();

		file << " // This file is auto generated please don't modify.\n";

		SerialCodeGenerate::IncludeHeader("ReflectStructs.h", file);
		SerialCodeGenerate::IncludeHeader("Core/Util.h", file);

		file << "\n";
		file << "#ifdef " + data.FileName + SerialFileHeaderGuard + "_h\n";
		file << "#error \"" + data.FileName + SerialFileHeaderGuard + ".h" + " already included, missing 'pragma once' in " + data.FileName + ".h\"\n";
		file << "#endif " + data.FileName + SerialFileHeaderGuard + "_h\n";
		file << "#define " + data.FileName + SerialFileHeaderGuard + "_h\n\n";

		WriteMacros(data, file, addtionalOptions);
	}

	void SerialCodeGenerateHeader::WriteMacros(const Reflect::FileParsedData& data, std::ofstream& file, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		for (const auto& reflectData : data.ReflectData)
		{
			const std::string CurrentFileId = GetCurrentFileID(data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine);

			WriteStaticClass(reflectData, file, CurrentFileId, addtionalOptions);
			WriteMemberProperties(reflectData, file, CurrentFileId, addtionalOptions);
			WriteFunctions(reflectData, file, CurrentFileId, addtionalOptions);
			WriteFunctionGet(reflectData, file, CurrentFileId, addtionalOptions);
			WriteMemberPropertiesOffsets(reflectData, file, CurrentFileId, addtionalOptions);
			WriteMemberGet(reflectData, file, CurrentFileId, addtionalOptions);

			WRITE_CURRENT_FILE_ID(data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine) + "_GENERATED_BODY \\\n";
			file << CurrentFileId + "_STATIC_CLASS \\\n";
			file << CurrentFileId + "_PROPERTIES \\\n";
			file << CurrentFileId + "_FUNCTION_DECLARE \\\n";
			file << CurrentFileId + "_FUNCTION_GET \\\n";
			file << CurrentFileId + "_PROPERTIES_OFFSET \\\n";
			file << CurrentFileId + "_PROPERTIES_GET \\\n";

			WRITE_CLOSE();
		}

		file << "#undef CURRENT_FILE_ID\n";
		file << "#define CURRENT_FILE_ID " + GetCurrentFileID(data.FileName) + "\n";
	}

	void SerialCodeGenerateHeader::WriteStaticClass(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_STATIC_CLASS \\\n";
		WRITE_PUBLIC();
		if (data.SuperName.length())
			file << "\ttypedef " + data.SuperName + " SuperClass;\\\n";
		file << "\tstatic const Reflect::Class StaticClass;\\\n";
		file << "\tstatic void __PlacementNew(" << data.Name << "* obj) { new(obj) " << data.Name << "; }\\\n";
		file << "\tstatic void __PlacementDelete(" << data.Name << "* obj) { obj->~" << data.Name << "(); }\\\n";
		WRITE_CLOSE();
	}

	void SerialCodeGenerateHeader::WriteMemberProperties(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_PROPERTIES \\\n";
		WRITE_PRIVATE();
		file << "\tstatic Reflect::ReflectMemberProp __REFLECT_MEMBER_PROPS__[" + std::to_string(data.Members.size()) + "];\\\n";
		WRITE_CLOSE();
	}

	void SerialCodeGenerateHeader::WriteMemberPropertiesOffsets(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_PROPERTIES_OFFSET \\\n";
		WRITE_PRIVATE();
		for (const auto& member : data.Members)
		{
			file << "\tstatic constexpr int __REFLECT__" + member.Name + "() { return offsetof(" + data.Name + ", " + member.Name + "); }; \\\n";
		}
		WRITE_CLOSE();
	}

	void SerialCodeGenerateHeader::WriteMemberGet(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_PROPERTIES_GET \\\n";
		WRITE_PUBLIC();
		file << "virtual Reflect::ReflectMember GetMember(const std::string_view& memberName) override;\\\n";
		file << "virtual std::vector<Reflect::ReflectMember> GetMembers(std::vector<std::string> const& flags) override;\\\n";
		WRITE_CLOSE();
	}

	void SerialCodeGenerateHeader::WriteFunctions(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		//TODO: Pass in parameters in someway. Prob need to use templates.

		auto populateArgs = [](const std::vector<Reflect::ReflectTypeNameData>& args) -> std::string
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

	void SerialCodeGenerateHeader::WriteFunctionGet(const Reflect::ReflectContainerData& data, std::ofstream& file, const std::string& currentFileId, const SerialCodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_FUNCTION_GET \\\n";
		WRITE_PUBLIC();
		file << "\tvirtual Reflect::ReflectFunction GetFunction(const std::string_view &functionName) override;\\\n";
		WRITE_CLOSE();
	}
}