#include "CodeGenerate/CodeGenerate.h"
#include "CodeGenerate/CodeGenerateHeader.h"
#include "CodeGenerate/CodeGenerateSource.h"
#include "Instrumentor.h"
#include <assert.h>
#include <filesystem>

namespace Reflect
{
	constexpr const char* ContainerPrefix = "ReflectObject";

	CodeGenerate::CodeGenerate()
	{ }

	CodeGenerate::~CodeGenerate()
	{ }

	void CodeGenerate::Reflect(const FileParsedData& data, const CodeGenerateAddtionalOptions& addtionalOptions)
	{
		REFLECT_PROFILE_FUNCTION();

		CodeGenerateHeader header;
		CodeGenerateSource source;

		const auto headerPath = data.FilePath + "/" + data.FileName + ReflectFileGeneratePrefix + "." + data.FileExtension;
		const auto sourceExtension = data.FileExtension == "h" ? "cpp" : "cxx";
		const auto sourcePath = addtionalOptions.OutputCPPDir + data.SubPath + "/" + data.FileName + ReflectFileGeneratePrefix + "." + sourceExtension;

		std::ostringstream sout;

		header.GenerateHeader(data, sout, addtionalOptions);
		WriteIfDifferent(headerPath, sout.str());
		sout.str("");

		// There is no source to write for now, so don't even try!
#if 0
		source.GenerateSource(data, sout, addtionalOptions);
		WriteIfDifferent(sourcePath, sout.str());
#else
		if (std::filesystem::exists(sourcePath))
		{
			std::filesystem::remove(sourcePath);
		}
#endif
		sout.str("");
	}

	void CodeGenerate::WriteIfDifferent(const std::string& filePath, const std::string& str)
	{
		std::ifstream fin(filePath);
		const std::string existing((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());

		if (existing == str)
		{
			// They're the same.
			// Don't write the updated file - changes the timestamp and makes compilers want to recompile them!
			return;
		}

		std::cout << "Writing: " << filePath << std::endl;

		fin.close();
		std::ofstream fout(filePath);
		fout.write(str.data(), str.length());
	}

	void CodeGenerate::IncludeHeader(const std::string& headerToInclude, std::ostream& file, bool windowsInclude)
	{
		if (windowsInclude)
		{
			file << "#include <" + headerToInclude + ">\n";
		}
		else
		{
			file << "#include \"" + headerToInclude + "\"\n";
		}
	}

	std::string CodeGenerate::GetMemberProps(const std::vector<std::string>& flags)
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
	}

	bool CodeGenerate::IsSerialised(const Reflect::ReflectMemberData& data)
	{
		const auto it = std::find_if(data.ContainerProps.begin(), data.ContainerProps.end(), [](const auto& p) { return p.find("Serialise") == 0; });
		return it != data.ContainerProps.end();
	}
}
