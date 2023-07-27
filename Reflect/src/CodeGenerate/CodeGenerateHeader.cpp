#include "CodeGenerate/CodeGenerateHeader.h"
#include "CodeGenerate/CodeGenerate.h"
#include "Instrumentor.h"
#include <assert.h>
#include <filesystem>

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
#define WRITE_PROTECTED() file << "protected:\\\n"
#define WRITE_PRIVATE() file << "private:\\\n"

	void CodeGenerateHeader::GenerateHeader(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();

		file << " // This file is auto generated please don't modify.\n";

		CodeGenerate::IncludeHeader("ReflectStructs.h", file);
		CodeGenerate::IncludeHeader("Core/Core.h", file);
		CodeGenerate::IncludeHeader("Core/Enums.h", file);
		CodeGenerate::IncludeHeader("Core/Util.h", file);

		const auto relativePath = data.FilePath + "\\" + std::string(data.FileName + "." + data.FileExtension);
		std::string reflectGuard = relativePath + ReflectFileHeaderGuard;
		std::replace(reflectGuard.begin(), reflectGuard.end(), '/', '_');
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
	}

	void CodeGenerateHeader::GenerateStaticHeader(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();

		file << " // This file is auto generated please don't modify.\n";

		CodeGenerate::IncludeHeader("ReflectStructs.h", file);
		CodeGenerate::IncludeHeader("Core/Core.h", file);
		CodeGenerate::IncludeHeader("Core/Enums.h", file);
		CodeGenerate::IncludeHeader("Core/Util.h", file);
		CodeGenerate::IncludeHeader("ReflectStatic.h", file);

		const auto relativePath = data.FilePath + "\\" + std::string(data.FileName + "." + data.FileExtension);
		std::string reflectGuard = relativePath + ReflectStaticFileHeaderGuard;
		std::replace(reflectGuard.begin(), reflectGuard.end(), '/', '_');
		std::replace(reflectGuard.begin(), reflectGuard.end(), '\\', '_');
		std::replace(reflectGuard.begin(), reflectGuard.end(), '.', '_');
		std::replace(reflectGuard.begin(), reflectGuard.end(), ':', '_');
		std::replace(reflectGuard.begin(), reflectGuard.end(), '-', '_');

		file << "\n";
		file << "#ifdef " + reflectGuard + "_h\n";
		file << "#error \"" + reflectGuard + ".h" + " already included, missing 'pragma once' in " + data.FileName + ".h\"\n";
		file << "#endif //" + reflectGuard + "_h\n";
		file << "#define " + reflectGuard + "_h\n\n";

		file << "namespace Reflect {\n\n";
		if (addtionalOptions.Namespace.length())
		{
			file << "\tusing namespace " << addtionalOptions.Namespace << ";\n\n";
		}

		for (const auto& reflectData : data.ReflectData)
		{
			if (reflectData.ReflectType != ReflectType::Enum)
			{
				WriteStatic(reflectData, data, file, addtionalOptions);
			}
		}

		file << "} // namespace Reflect\n\n";
	}

	void CodeGenerateHeader::WriteMacros(const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		for (const auto& reflectData : data.ReflectData)
		{
			const std::string CurrentFileId = GetCurrentFileID(addtionalOptions, data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine);

			if (reflectData.ReflectType == ReflectType::Enum)
			{
				WriteEnumMacros(reflectData, data, file, CurrentFileId, addtionalOptions);
			}
			else
			{
				WriteClassMacros(reflectData, data, file, CurrentFileId, addtionalOptions);
			}

			WRITE_CLOSE();
		}

		file << "#undef CURRENT_FILE_ID\n";
		file << "#define CURRENT_FILE_ID " + GetCurrentFileID(addtionalOptions, data.FileName) + "\n";
	}

	void CodeGenerateHeader::WriteStatic(const Reflect::ReflectContainerData& reflectData, const FileParsedData& data, std::ostream& file, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		auto qualifiedContainerType = addtionalOptions.Namespace.length()
			? addtionalOptions.Namespace + "::" + reflectData.Name
			: reflectData.Name;

		if (reflectData.TemplateArgNames.size())
		{
			qualifiedContainerType += "<";
			for (const auto& t : reflectData.TemplateArgNames)
			{
				qualifiedContainerType += t;
				if (&t != &reflectData.TemplateArgNames.back())
				{
					qualifiedContainerType += ", ";
				}
			}
			qualifiedContainerType += ">";
		}

		file << "\ttemplate<" << reflectData.TemplateArgString << "> struct ReflectStatic<" << qualifiedContainerType;
		file << "> {\n";

		file << "\t\tstatic inline constexpr auto Properties = std::make_tuple(\n";
		for (const auto& p : reflectData.Members)
		{
			file << "\t\t\tmake_static_field<" << p.Type << ">(\"" << p.Name << "\", " << qualifiedContainerType << "::__OFFSETOF__" << p.Name << "(), std::make_tuple(";
			for (const auto& f : p.ContainerProps)
			{
				file << "\"" << f << "\"";
				if (&f != &p.ContainerProps.back())
				{
					file << ", ";
				}
			}
			file << ")";

			file << ")";
			if (&p != &reflectData.Members.back())
			{
				file << ", ";
			}
			file << "\n";
		}
		file << "\t\t);\n";

		file << "\t};\n\n";
	}

	void CodeGenerateHeader::WriteClassMacros(const Reflect::ReflectContainerData& reflectData, const FileParsedData& data, std::ostream& file, const std::string& CurrentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		WriteMemberPropertiesOffsets(reflectData, file, CurrentFileId, addtionalOptions);
		WriteStaticClass(reflectData, file, CurrentFileId, addtionalOptions);
		WriteMemberProperties(reflectData, file, CurrentFileId, addtionalOptions);
		WriteFunctions(reflectData, file, CurrentFileId, addtionalOptions);

		WRITE_CURRENT_FILE_ID(data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine) + "_GENERATED_BODY \\\n";
		file << CurrentFileId + "_PROPERTIES_OFFSET \\\n";
		file << CurrentFileId + "_PROPERTIES \\\n";
		file << CurrentFileId + "_FUNCTION_DECLARE \\\n";
		file << CurrentFileId + "_STATIC_CLASS \\\n";
	}

	void CodeGenerateHeader::WriteEnumMacros(const Reflect::ReflectContainerData& reflectData, const FileParsedData& data, std::ostream& file, const std::string& CurrentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		if (!Util::ContainsProperty(reflectData.ContainerProps, { "ValueType" }))
		{
			file << "#error \"Enum " << reflectData.Name << " is missing ValueType\"\n";
			return;
		}

		WriteStaticEnum(reflectData, file, CurrentFileId, addtionalOptions);
		WriteEnumOperators(reflectData, file, CurrentFileId, addtionalOptions);
		WriteEnumValues(reflectData, file, CurrentFileId, addtionalOptions);
		WriteEnumMembers(reflectData, file, CurrentFileId, addtionalOptions);
		WriteEnumMethods(reflectData, file, CurrentFileId, addtionalOptions);

		WRITE_CURRENT_FILE_ID(data.FileName) + "_" + std::to_string(reflectData.ReflectGenerateBodyLine) + "_GENERATED_BODY \\\n";
		file << CurrentFileId + "_STATIC_ENUM \\\n";
		file << CurrentFileId + "_OPERATORS \\\n";
		file << CurrentFileId + "_VALUES \\\n";
		file << CurrentFileId + "_MEMBERS \\\n";
		file << CurrentFileId + "_METHODS \\\n";
	}

	void CodeGenerateHeader::WriteStaticClass(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_STATIC_CLASS \\\n";
		WRITE_PUBLIC();
		file << "\tusing ThisClass = " + data.Name + ";\\\n";
		if (data.SuperName.length())
			file << "\tusing SuperClass = " + data.SuperName + ";\\\n";
		file << "\tfriend struct Reflect::ReflectStatic<" << data.Name << ">;\\\n";

		const auto hasAllocator = !Util::ContainsProperty(data.ContainerProps, { "Abstract" });
		const std::string_view allocatorParm = hasAllocator ? "" : "nullptr";

		file << "\tstatic inline const Reflect::Class StaticClass = Reflect::Class( \\\n"
			<< "\t\tReflect::Util::GetTypeName<ThisClass>(), \\\n"
			<< "\t\t" << (data.SuperName != "Reflect::IReflect" ? (std::string("&") + data.SuperName + "::StaticClass") : std::string("nullptr")) << ", \\\n"
			<< "\t\tReflect::ClassAllocator::Create<" << data.Name << ">(" << allocatorParm << "), \\\n"
			<< "\t\t" << CodeGenerate::GetMemberProps(data.ContainerProps) << ", \\\n"
			<< CreateMemberInitializerList(data) << ", \\\n"
			<< CreateFunctionInitializerList(data) << ", \\\n"
			<< CreateInterfaceList(data) << " \\\n"
			<< "\t); \\\n";

		file << "\tconst Reflect::Class& GetClass() const override { return StaticClass; }\\\n";

		WRITE_PRIVATE();
		file << "\tstatic inline const Reflect::LinkClass Linker = StaticClass; \\\n";
		file << "\tconst Reflect::LinkClass& GetLinkClass() const override { return Linker; }\\\n";

		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteMemberProperties(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_PROPERTIES \\\n";
		WRITE_PUBLIC();
		for (const auto& member : data.Members)
		{
			file << "\tstatic constexpr const char* nameof_" << member.Name << " = \"" << member.Name << "\";\\\n";
		}
		
		WRITE_PRIVATE();
		if (data.Members.size())
		{
			// Write wrappers for the IO functions.
			// I think these are necessary because they use the __OFFSETOF__ constexprs.
			// The actual property table isn't happy taking a function pointer to the main ReadField+WriteField functions
			// because they need to be instantiated with the __OFFSETOF__ constexprs, but it's happy to have an untemplated
			// function that uses __OFFSETOF__ internally, because it doens't need to evaluate it.
			for (const auto& member : data.Members)
			{
				if (!CodeGenerate::IsSerialised(member))
				{
					continue;
				}
				
				const std::string readField = "DispatchReadField<Reflect::IUnserialiser, " + member.Type + ">";
				const std::string writeField = "DispatchWriteField<Reflect::ISerialiser, " + member.Type + ">";

				file << "\tstatic void __READ__" << member.Name << "(Reflect::IUnserialiser& u, std::istream& in, void* field) { " << readField << "(u, in, *reinterpret_cast<" << member.Type << "*>(field)); }\\\n";
				file << "\tstatic void __WRITE__" << member.Name << "(Reflect::ISerialiser& s, std::ostream& out, const void* field) { " << writeField << "(s, out, *(reinterpret_cast<const " << member.Type << "*>(field))); }\\\n";
			}
		}

		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteMemberPropertiesOffsets(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_PROPERTIES_OFFSET \\\n";
		WRITE_PRIVATE();
		for (const auto& member : data.Members)
		{
			file << "\tstatic constexpr int __OFFSETOF__" + member.Name + "() { return offsetof(" + data.Name + ", " + member.Name + "); }; \\\n";
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
		WRITE_PUBLIC();
			file << "\tvoid Serialise(Reflect::ISerialiser &s, std::ostream& out) const override { DispatchSerialise(s, out, *this); }\\\n";
			file << "\tvoid Unserialise(Reflect::IUnserialiser& u, std::istream& in) override    { DispatchUnserialise(u, in, *this); }\\\n";
		WRITE_PRIVATE();
		for (const auto& func : data.Functions)
		{
			file << "\tstatic Reflect::ReflectReturnCode __REFLECT_FUNC__" + CleanFunctionName(func.Name) + "(void* objectPtr, void* returnValuePtr, Reflect::FunctionPtrArgs& functionArgs)\\\n";
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

	std::string CodeGenerateHeader::CreateMemberInitializerList(const ReflectContainerData& data)
	{
		std::ostringstream oss;

		if (data.Members.size())
		{
			oss << "\t\tReflect::Util::make_vector<Reflect::ReflectMemberProp>( \\\n";
			for (const auto& member : data.Members)
			{
				const auto isLast = &member == &data.Members.back();
				const auto eol = isLast ? "\\\n" : ", \\\n";

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

				oss << "\t\t\tReflect::CreateReflectMemberProp<" + member.Type + ">(\"" + member.Name + "\", Reflect::Util::GetTypeName<" + member.Type + ">(), __OFFSETOF__" + member.Name + "(), " + CodeGenerate::GetMemberProps(member.ContainerProps) + ", " + readField + ", " + writeField + ")" + eol;
			}
			oss << "\t\t)";
		}
		else
		{
			oss << "\t\t{}";
		}

		return oss.str();
	}

	std::string CodeGenerateHeader::CreateFunctionInitializerList(const ReflectContainerData& data)
	{
		std::ostringstream oss;

		if (data.Functions.size())
		{
			oss << "\t\tReflect::Util::make_vector<Reflect::ReflectMemberFunction>( \\\n";
			for (const auto& func : data.Functions)
			{
				const auto isLast = &func == &data.Functions.back();
				const auto eol = isLast ? "\\\n" : ", \\\n";

				oss << "\t\t\tReflect::ReflectMemberFunction(\"" + func.Name + "\", __REFLECT_FUNC__" + CleanFunctionName(func.Name) + ")" + eol;
			}
			oss << "\t\t)";
		}
		else
		{
			oss << "\t\t{}";
		}

		return oss.str();
	}

	std::string CodeGenerateHeader::CreateInterfaceList(const ReflectContainerData& data)
	{
		std::ostringstream oss;

		if (data.Interfaces.size())
		{
			oss << "\t\tReflect::Util::make_vector<std::string>( \\\n";
			for (const auto& name : data.Interfaces)
			{
				const auto isLast = &name == &data.Interfaces.back();
				const auto eol = isLast ? "\\\n" : ", \\\n";

				oss << "\t\t\tReflect::Util::GetTypeName<" + name + ">()" + eol;
			}
			oss << "\t\t)";
		}
		else
		{
			oss << "\t\t{}";
		}

		return oss.str();
	}

	std::string CodeGenerateHeader::CleanFunctionName(const std::string_view& name)
	{
		std::string clean = (std::string)name;

		// I don't see a need to support others, for now...
		clean = Util::replace_all(clean, "()", "_FUNCTOR");

		return clean;
	}

	void CodeGenerateHeader::WriteStaticEnum(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		if (!Util::ContainsProperty(data.ContainerProps, { "ValueType" }))
		{
			file << "#error \"Enum " << data.Name << " is missing ValueType\"\n";
			return;
		}

		std::string_view valueType;
		Util::TryGetPropertyValue(data.ContainerProps, "ValueType", valueType);

		file << "#define " + currentFileId + "_STATIC_ENUM \\\n";
		WRITE_PUBLIC();
		file << "\tusing ThisClass = " + data.Name + ";\\\n";
		file << "\tusing SuperClass = " + data.SuperName + ";\\\n";
		file << "\tusing ValueType = " << valueType << ";\\\n";
		file << "\tconstexpr " << data.Name << "() {}\\\n";
		file << "\tconstexpr " << data.Name << "(Values v) : Value(v) {}\\\n";

		WRITE_PRIVATE();
		file << "\tstatic Reflect::Enum::ConstantType __GetValue__(const void* ptr) { return ((" << data.Name << "*)ptr)->Value; } \\\n";
		file << "\tstatic void __SetValue__(void* ptr, Reflect::Enum::ConstantType value) { ((" << data.Name << "*)ptr)->Value = (Values)value; } \\\n";
		
		WRITE_PUBLIC();
		file << "\tstatic inline const Reflect::Enum StaticEnum = Reflect::Enum(\"" << data.Name << "\", Reflect::Util::GetTypeName<ValueType>(), \\\n";
		file << "\t\t" << CodeGenerate::GetMemberProps(data.ContainerProps) << ", \\\n";

		file << "\t\t{ \\\n";
		for (const auto& c : data.Constants)
		{
			std::string_view displayLabel;
			if (!Util::TryGetPropertyValue(c.Flags, "DisplayLabel", displayLabel))
			{
				displayLabel = c.Name;
			}

			file << "\t\t\tReflect::EnumConstant(\"" << c.Name << "\", " << c.Value << ", \"" << displayLabel << "\", " << CodeGenerate::GetMemberProps(c.Flags) << "), \\\n";
		}
		file << "\t\t}, \\\n";

		file << "\t\t__GetValue__, __SetValue__ \\\n";

		file << "\t);\\\n\n";

		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteEnumOperators(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_OPERATORS \\\n";
		WRITE_PUBLIC();
		// Allow switch and comparisons.
		file << "\tconstexpr operator Values() const { return Values(Value); }\\\n";

		// Prevent usage: if(value)
		file << "\texplicit operator bool() const = delete;\\\n";

		file << "\tconstexpr bool operator==(Values rhs) const { return Value == rhs; }\\\n";
		file << "\tconstexpr bool operator!=(Values rhs) const { return Value != rhs; }\\\n";
		file << "\tconstexpr auto& operator=(Values v) { Value = v; return *this; }\\\n";
		file << "\tconstexpr auto& operator|=(Values rhs) { Value = (Values)(Value | rhs); return *this; }\\\n";
		file << "\tconstexpr auto& operator|=(const " << data.Name << "& rhs) { Value = (Values)(Value | rhs.Value); return *this; }\\\n";
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteEnumValues(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_VALUES \\\n";
		WRITE_PUBLIC();

		file << "\tstatic constexpr std::array<std::pair<std::string_view, Values>, " << data.Constants.size() << "> Names{\\\n";
		for (const auto& c : data.Constants)
		{
			file << "\t\tstd::pair{ \"" << c.Name << "\", Values(" << c.Value << ") },\\\n";
		}
		file << "\t};\\\n";

		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteEnumMembers(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_MEMBERS \\\n";
		WRITE_PROTECTED();

		// To get a better debugging experience - define the real variable as the enum.
		// The load()+store() methods below worry about converting to and from the ValueType.
		// Serialisers should use that load()+store() interface.
		file << "\tValues Value = (Values)0;\\\n";
	
		WRITE_CLOSE();
	}

	void CodeGenerateHeader::WriteEnumMethods(const ReflectContainerData& data, std::ostream& file, const std::string& currentFileId, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		file << "#define " + currentFileId + "_METHODS \\\n";
		WRITE_PUBLIC();

		file << "\tstatic const auto & ToString(" << data.Name << " v) { return StaticEnum.ToString(v); }\\\n";
		file << "\tconst auto& ToString() const { return StaticEnum.ToString(Value); }\\\n";

		file << "\tstatic auto Parse(const std::string_view& value)		{ return " << data.Name << "((Values)const_cast<Reflect::Enum&>(StaticEnum).Parse(value)); }\\\n";
		file << "\tstatic auto TryParse(const std::string_view& value, " << data.Name << "& v)	{ return const_cast<Reflect::Enum&>(StaticEnum).TryParse(value, v.Value); }\\\n";
		
		file << "\tstatic auto ContainsProperty(std::vector<std::string> const& flags)	{ return StaticEnum.ContainsProperty(flags); }\\\n";
		file << "\tstatic auto GetPropertyValue(const std::string_view& flag)	{ return StaticEnum.GetPropertyValue(flag); }\\\n";
		file << "\tstatic auto TryGetPropertyValue(const std::string_view& flag, std::string_view& value)	{ return StaticEnum.TryGetPropertyValue(flag, value); }\\\n";

		file << "\tconst auto& GetConstant() const	{ return StaticEnum.GetConstant(Value); }\\\n";

		file << "\tstd::string ToBitfieldString() const {\\\n";
		file << "\t\tstd::string s;\\\n";
		file << "\t\tfor (const auto& it : StaticEnum.Values) {\\\n";
		file << "\t\t\tif (Value & it.Value) {\\\n";
		file << "\t\t\t\ts.reserve(s.length() + it.Name.length() + 1);\\\n";
		file << "\t\t\t\tif (s.length()) {\\\n";
		file << "\t\t\t\t\ts += '|';\\\n";
		file << "\t\t\t\t}\\\n";
		file << "\t\t\t\ts += it.Name;\\\n";
		file << "\t\t\t}\\\n";
		file << "\t\t}\\\n";
		file << "\t\treturn s;\\\n";
		file << "\t}\\\n";

		file << "\tstatic Values ParseBitfieldString(const std::string_view& values)	{ return (Values)StaticEnum.ParseBitfieldString(values); }\\\n";

		file << "\tValueType	load() const	{ return Value; }\\\n";
		file << "\tvoid	store(ValueType v)		{ Value = (Values)v; }\\\n";
	
		WRITE_CLOSE();
	}
}