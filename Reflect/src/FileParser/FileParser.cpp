#include "FileParser/FileParser.h"
#include "FileParser/FileParserKeyWords.h"
#include "Instrumentor.h"
#include <sstream>
#include <vector>
#include <iostream>
#include <filesystem>
#include <stack>
#include <assert.h>
#include <string.h>

namespace Reflect
{
	constexpr int DEFAULT_TYPE_SIZE = 0;

	const std::vector<char> emptyChars = { '\n', '\t', '\r', ' ' };
	const std::vector<char> generalEndChars = { ' ', '(', '=', ';', ':' };
	const std::vector<char> functionStartChars = { '(' };
	const std::vector<char> memberStartChars = { '=', ';' };

	FileParser::FileParser()
	{ }

	FileParser::~FileParser()
	{ }

	void FileParser::ParseDirectory(const std::string& directory)
	{
		REFLECT_PROFILE_FUNCTION();

		m_filesParsed.clear();
		m_filesToRemove.clear();

		std::filesystem::path dirPath(directory);
		std::error_code err;
		if (!std::filesystem::is_directory(dirPath, err))
		{
			std::cout << err.message() << '\n';
			return;
		}

		for (const auto& f : std::filesystem::recursive_directory_iterator(directory))
		{
			std::string filePath = f.path().u8string();

			if ((f.is_regular_file() || f.is_character_file()) &&
				CheckExtension(filePath, { ".h", ".hpp" }) &&
				!CheckIfAutoGeneratedFile(filePath))
			{
				// TODO thread this. We could load files on more than one thread to speed
				// this up.
				std::cout << "Parsing: " << filePath << std::endl;
				std::ifstream file = OpenFile(filePath);
				FileParsedData data = LoadFile(file);
				data.FileName = f.path().filename().u8string().substr(0, f.path().filename().u8string().find_last_of('.'));
				data.FilePath = f.path().parent_path().u8string();
				m_filesParsed.push_back(data);
				CloseFile(file);
			}
		}


		// All files have been loaded.
		// Now we need to parse them to find all the information we want from them.
		// TODO this could also be threaded.
		for (auto& file : m_filesParsed)
		{
			if (!ParseFile(file))
			{
				m_filesToRemove.push_back(file.FileName);
			}
		}

		for (auto const& fileToRemove : m_filesToRemove)
		{
			auto itr = std::find_if(m_filesParsed.begin(), m_filesParsed.end(), [fileToRemove](FileParsedData const& data)
			{
				return fileToRemove == data.FileName;
			});
			assert(itr != m_filesParsed.end() && "[FileParser::ParseDirectory] Remove file to parse dose not exists.");
			m_filesParsed.erase(itr);
		}
	}

	void FileParser::SetIgnoreStrings(const std::vector<std::string>& ignoreStrings)
	{
		m_ignoreStrings = ignoreStrings;
	}

	std::ifstream FileParser::OpenFile(const std::string& filePath)
	{
		std::ifstream file = std::ifstream(filePath);
		assert(file.is_open() && "[FileParser::OpenFile] File is not open.");
		return file;
	}

	void FileParser::CloseFile(std::ifstream& file)
	{
		if (file.is_open())
		{
			file.close();
		}
	}

	bool FileParser::CheckExtension(const std::string& filePath, std::vector<const char*> extensions)
	{
		std::string extension = filePath.substr(filePath.find_last_of('.'));
		for (auto& e : extensions)
		{
			if (e == extension)
			{
				return true;
			}
		}
		return false;
	}

	bool FileParser::CheckIfAutoGeneratedFile(const std::string& filePath)
	{
		return filePath.find(ReflectFileGeneratePrefix) != std::string::npos;
	}

	FileParsedData FileParser::LoadFile(std::ifstream& file)
	{
		FileParsedData data = {};

		file.seekg(0, std::ios::end);
		int fileSize = static_cast<int>(file.tellg());
		file.seekg(0, std::ios::beg);
		data.Data = std::string(fileSize, '\0');
		data.Cursor = 0;
		file.read(&data.Data[0], fileSize);

		return data;
	}

	bool FileParser::ParseFile(FileParsedData& fileData)
	{
		REFLECT_PROFILE_FUNCTION();

		bool reflectItem = false;
		while (ReflectContainerHeader(fileData, RefectStructKey, EReflectType::Struct) || ReflectContainerHeader(fileData, RefectClassKey, EReflectType::Class))
		{
			ReflectContainer(fileData);
			reflectItem = true;
		}
		return reflectItem;
	}

	bool FileParser::ReflectContainerHeader(FileParsedData& fileData, const std::string& keyword, const EReflectType type)
	{
		// Check if we can reflect this class/struct. 
		int reflectStart = static_cast<int>(fileData.Data.find(keyword, fileData.Cursor));
		if (reflectStart == std::string::npos)
		{
			// Can't reflect this class/struct. Return.
			return false;
		}

		ReflectContainerData containerData = {};

		containerData.ReflectType = type;
		fileData.Cursor = reflectStart + static_cast<int>(keyword.length()) + 1;
		containerData.ContainerProps = ReflectFlags(fileData);

		if (containerData.ReflectType == EReflectType::Class)
		{
			int newPos = (int)fileData.Data.find("class", fileData.Cursor);
			if (newPos != std::string::npos)
			{
				fileData.Cursor = newPos;
				fileData.Cursor += 5;
			}
			else
			{
				return false;
			}
		}
		else if (containerData.ReflectType == EReflectType::Struct)
		{
			int newPos = (int)fileData.Data.find("struct", fileData.Cursor);
			if (newPos != std::string::npos)
			{
				fileData.Cursor = newPos;
				fileData.Cursor += 6;
			}
			else
			{
				return false;
			}
		}

		// Get the flags passed though the REFLECT macro.
		std::string containerName;
		while (fileData.Data.at(fileData.Cursor) != ':' && fileData.Data.at(fileData.Cursor) != '{' && fileData.Data.at(fileData.Cursor) != '\n')
		{
			if (fileData.Data.at(fileData.Cursor) != ' ')
			{
				containerName += fileData.Data.at(fileData.Cursor);
			}
			++fileData.Cursor;
		}
		containerData.Name = containerName;
		containerData.Type = containerName;
		containerData.TypeSize = DEFAULT_TYPE_SIZE;
		fileData.ReflectData.push_back(containerData);

		return true;
	}

	void FileParser::ReflectContainer(FileParsedData& fileData)
	{
		int endOfContainerCursor = FindEndOfConatiner(fileData);

		// Good, we have a reflected container class/struct.
		// First find out which it is and verify that we are inheriting from "ReflectObject".
		std::stack<char> bracketStack;
		ReflectContainerData& conatinerData = fileData.ReflectData.back();

		int generatedBodyLine = static_cast<int>(fileData.Data.find(ReflectGeneratedBodykey, fileData.GeneratedBodyLineOffset));
		assert(generatedBodyLine != -1 && "[FileParser::ReflectContainer] 'REFLECT_GENERATED_BODY()' is missing from a container.");
		fileData.GeneratedBodyLineOffset = generatedBodyLine + static_cast<int>(strlen(ReflectGeneratedBodykey));
		conatinerData.ReflectGenerateBodyLine = CountNumberOfSinceTop(fileData, generatedBodyLine, '\n') + 1;

		// Set us to the start of the class/struct. We should continue until we find something.
		char c = FindNextChar(fileData, '{');
		std::vector<std::string> reflectFlags;
		while (true)
		{
#ifdef EXP_PARSER
			if (CheckForEndOfFile(fileData, endOfContainerCursor))
				break;

			c = FindNextChar(fileData, emptyChars);
			std::string word = FindNextWord(fileData, generalEndChars);
			if (CheckForTypeAlias(word))
			{
				FindNextChar(fileData, ';');
				continue;
			}

			// Check for child strcuts/class

			if (CheckForVisibility(word))
			{
				++fileData.Cursor;
				continue;
			}

			// Check for constructor/destructor
			if (CheckForConstructor(fileData, conatinerData, word))
				continue;

			if (CheckForIgnoreWord(fileData, word))
				continue;

			RemoveComments(fileData, word);

			// We should now have a type (unless there are macros or compiler keywords).
			if (IsWordReflectKey(word))
			{
				if (word == ReflectGeneratedBodykey)
				{
					c = FindNextChar(fileData, { '(', ')' });
					continue;
				}
				else if (word == ReflectPropertyKey)
				{
					// Get the flags for the property
					reflectFlags = ReflectFlags(fileData);
					continue;
				}
			}
			else
			{
				// We think we have function or memeber.
				fileData.Cursor -= (int)word.size();
			}

			if (CheckForEndOfFile(fileData, endOfContainerCursor))
				break;

			EReflectType refectType = CheckForReflectType(fileData);
			if (refectType == EReflectType::Member)
			{
				conatinerData.Members.push_back(GetMember(fileData, reflectFlags));
			}
			else if (refectType == EReflectType::Function)
			{
				conatinerData.Functions.push_back(GetFunction(fileData, reflectFlags));
			}
			//else
			//{
			//	assert(false);
			//	continue;
			//}
			reflectFlags = {};
#else
			int reflectStart = static_cast<int>(fileData.Data.find(PropertyKey, fileData.Cursor));
			if (reflectStart == static_cast<int>(std::string::npos) || reflectStart > endOfContainerCursor)
			{
				// There are no more properties to reflect or we have found a new container to reflect.
				break;
			}
			fileData.Cursor = reflectStart + static_cast<int>(strlen(PropertyKey));

			// Get the reflect flags.
			auto propFlags = ReflectFlags(fileData);

			// Get the type and name of the property to reflect.
			auto [type, name, isConst] = ReflectTypeAndName(fileData, {});

			char c = FindNextChar(fileData, { ' ' });
			bool defaultMemberValue = c == '=';
			while (c != ';' && (c != '(' || defaultMemberValue) && c != '\n')
			{
				++fileData.Cursor;
				c = FindNextChar(fileData, { ' ', '=' });
			}

			// Find out if the property is a function or member variable.
			if (c == ';')
			{
				// Member
				// We have found a member variable 
				ReflectMemberData memberData = {};
				memberData.Type = type;
				memberData.Name = name;
				memberData.ReflectValueType = type.back() == '*' || type.back() == '&' ? (type.back() == '*' ? ReflectValueType::Pointer : ReflectValueType::Reference) : ReflectValueType::Value;
				memberData.TypeSize = DEFAULT_TYPE_SIZE;
				memberData.ContainerProps = propFlags;
				memberData.IsConst = isConst;
				conatinerData.Members.push_back(memberData);
			}
			else if (c == '(')
			{
				ReflectFunctionData funcData = {};
				funcData.Type = type;
				funcData.Name = name;
				funcData.TypeSize = DEFAULT_TYPE_SIZE;
				funcData.ContainerProps = propFlags;
				conatinerData.Functions.push_back(funcData);

				// Function
				ReflectGetFunctionParameters(fileData);
			}
			else if (c == '\n')
			{
				assert(false && "[FileParser::ParseFile] Unknown reflect type. This must be a member variable or function. Make sure ')' or ';' is used before a new line.");
			}

			++fileData.Cursor;
#endif
		}
	}

	int FileParser::FindEndOfConatiner(const FileParsedData& fileData)
	{
		int cursor = fileData.Cursor;
		char lastCharacter = '\0';
		char c = '\0';
		bool foundStartOfContainer = false;
		std::stack<char> symbols;
		while (true)
		{
#ifdef EXP_PARSER
			if (!foundStartOfContainer)
			{
				if (c == '{')
				{
					symbols.push(c);
					foundStartOfContainer = true;
				}
			}
			else
			{
				if (c == '{')
				{
					symbols.push(c);
				}
				else if (c == '}')
				{
					symbols.pop();
				}

				if (symbols.size() == 0)
				{
					char nextChar = FindNextChar(fileData, cursor, emptyChars);
					assert(nextChar == ';' && "[FileParser::FindEndOfConatiner] Did not find ';' at end of the container.");
					break;
				}
			}
#else
			if (lastCharacter == '}' && c == ';')
			{
				break;
			}

			if (c != '\t' && c != '\n')
			{
				lastCharacter = c;
			}
#endif
			++cursor;
			c = fileData.Data[cursor];
		}
		return cursor;
	}

	std::vector<std::string> FileParser::ReflectFlags(FileParsedData& fileData)
	{
		// Get the flags passed though the REFLECT macro.
		std::string flag;
		std::vector<std::string> flags;

		if (fileData.Data[fileData.Cursor] == '(')
		{
			++fileData.Cursor;
		}

		while (fileData.Data[fileData.Cursor] != ')')
		{
			char c = fileData.Data[fileData.Cursor];
			if (c == ',')
			{
				if (!flag.empty())
				{
					flags.push_back(flag);
				}
				flag = "";
			}
			else
			{
				if (c != ' ' && c != '\t')
				{
					flag += c;
				}
			}
			++fileData.Cursor;
		}
		++fileData.Cursor;
		if (!flag.empty())
		{
			flags.push_back(flag);
		}

		return flags;
	}

	char FileParser::FindNextChar(FileParsedData const& fileData, int& cursor, const std::vector<char>& ignoreChars)
	{
		FileParsedData copyFileData = fileData;
		copyFileData.Cursor = cursor;
		char c = FindNextChar(copyFileData, ignoreChars);
		cursor = copyFileData.Cursor;
		return c;
	}

	char FileParser::FindNextChar(FileParsedData& fileData, char charToFind)
	{
		char c = fileData.Data[fileData.Cursor];
		while (c != charToFind)
		{
			if (++fileData.Cursor < fileData.Data.size())
				c = fileData.Data[fileData.Cursor];
			else
				break;
		}
		return c;
	}

	char FileParser::FindNextChar(FileParsedData& fileData, const std::vector<char>& ignoreChars)
	{
		++fileData.Cursor;
		while (std::find(ignoreChars.begin(), ignoreChars.end(), fileData.Data[fileData.Cursor]) != ignoreChars.end())
		{
			++fileData.Cursor;
		}
		return fileData.Data[fileData.Cursor];
	}

	std::string FileParser::FindNextWord(FileParsedData& fileData, const std::vector<char>& endChars)
	{
		std::string s;
		s += fileData.Data[fileData.Cursor];
		char c = FindNextChar(fileData, std::vector<char>());
		while (std::find(endChars.begin(), endChars.end(), c) == endChars.end())
		{
			s += c;
			c = FindNextChar(fileData, std::vector<char>());
		}
		return s;
	}

	bool FileParser::IsWordReflectKey(std::string_view view)
	{
		return view == ReflectGeneratedBodykey ||
			view == ReflectPropertyKey;
	}

	bool FileParser::CheckForTypeAlias(std::string_view view)
	{
		return view == TypedefKey ||
			view == UsingKey;
	}

	bool FileParser::CheckForVisibility(std::string_view view)
	{
		return view == PublicKey ||
			view == ProtectedKey ||
			view == PrivateKey;
	}

	bool FileParser::CheckForConstructor(FileParsedData& fileData, ReflectContainerData& container, std::string_view view)
	{
		if (view == container.Name || view.at(0) == '~')
		{
			SkipFunctionBody(fileData);
			return true;
		}
		return false;
	}

	bool FileParser::CheckForIgnoreWord(FileParsedData& fileData, std::string_view view)
	{
		for (const std::string& str : m_ignoreStrings)
		{
			if (view == str)
			{
				FindNextChar(fileData, ';');
				return true;
			}
		}
		return false;
	}

	void FileParser::RemoveComments(FileParsedData& fileData, std::string& line)
	{
		// Remove all contents of a line with comments.
		size_t index = line.find("//");
		if (index != std::string::npos)
		{
			line = line.substr(0, index);
		}
	}

	void FileParser::GetReflectNameAndReflectValueTypeAndReflectModifer(std::string& str, std::string& name, EReflectValueType& valueType, EReflectValueModifier& modifer)
	{
		name = Util::Reverse(name);
		Util::RemoveCharAll(name, ' ');
		Util::RemoveString(str, name);

		valueType = CheckForRefOrPtr(str);
		Util::RemoveCharAll(str, '&');
		Util::RemoveCharAll(str, '*');

		modifer = CheckForMemberModifers(str);
		Util::RemoveString(str, ConstKey);
		Util::RemoveString(str, StaticKey);
		Util::RemoveString(str, VolatileKey);
		Util::RemoveString(str, VirtualKey);
	}

	ReflectFunctionData FileParser::GetFunction(FileParsedData& fileData, const std::vector<std::string>& flags)
	{
		ReflectFunctionData functionData;

		FileParsedData copy = fileData;
		FindNextChar(copy, ';');
		int endCursor = copy.Cursor;
		copy = fileData;
		FindNextChar(copy, '{');
		if (copy.Cursor < endCursor)
		{
			SkipFunctionBody(copy);
			endCursor = copy.Cursor;
		}
		else
			endCursor = std::min(copy.Cursor, endCursor);

		std::string line = fileData.Data.substr(fileData.Cursor, endCursor - fileData.Cursor);
		int endOfLineCursor = endCursor;

		uint32_t cBracket = (uint32_t)line.find_last_of(')');
		size_t functionConst = (uint32_t)line.find(ConstKey, cBracket);
		if (functionConst != std::string::npos)
		{
			functionData.IsConst = true;
			line = line.substr(0, cBracket + 1);
		}
		uint32_t oBracket = (uint32_t)line.find_first_of('(');
		std::string prameters = line.substr(oBracket, cBracket);
		Util::RemoveString(line, prameters);
		// Parse the parameters.
		functionData.Parameters = ReflectGetFunctionParameters(prameters);

		// Make sure if there are any spaces between the parameters and the function name we check for them.
		uint32_t cursor = (uint32_t)line.size() - 1;
		while (std::find(emptyChars.begin(), emptyChars.end(), line.at(cursor)) != emptyChars.end())
			--cursor;

		while (std::find(emptyChars.begin(), emptyChars.end(), line.at(cursor)) == emptyChars.end())
		{
			functionData.Name += line.at(cursor);
			--cursor;
		}
		// We should now have just the type.
		// TODO: Think about how to handle 'inline' modifiers and suck.
		// TODO: template support?
		GetReflectNameAndReflectValueTypeAndReflectModifer(line, functionData.Name, functionData.ReflectValueType, functionData.ReflectModifier);

		// Make sure there are no empty chars in the type string.
		for (const char& c : emptyChars)
			Util::RemoveCharAll(line, c);
		functionData.Type = line;

		fileData.Cursor = endOfLineCursor;
		return functionData;
	}

	ReflectMemberData FileParser::GetMember(FileParsedData& fileData, const std::vector<std::string>& flags)
	{
		ReflectMemberData memberData;
		memberData.TypeSize = DEFAULT_TYPE_SIZE;
		memberData.ContainerProps = flags;

		FileParsedData copy = fileData;
		FindNextChar(copy, ';');
		std::string line = fileData.Data.substr(fileData.Cursor, copy.Cursor - fileData.Cursor);
		int endOfLineCursor = fileData.Cursor + (int)line.size();
		line += ';';

		// Check for if there is a deault value being set.
		uint32_t equalCursor = (uint32_t)line.find('=');
		uint32_t semicolonCursor = (uint32_t)line.find(';');
		uint32_t cursor = equalCursor < semicolonCursor ? equalCursor : semicolonCursor;

		// Always go back one so we are not on '=' or ';';
		--cursor;
		// Go back untill we are not on an empty char.
		while (std::find(emptyChars.begin(), emptyChars.end(), line.at(cursor)) != emptyChars.end())
			--cursor;
		line = line.substr(0, cursor + 1);

		// 'line' should now contain the value, member name and any modifers like const.
		std::vector<char> endChars = emptyChars;
		endChars.push_back('*');
		endChars.push_back('&');
		while (std::find(endChars.begin(), endChars.end(), line.at(cursor)) == endChars.end())
		{
			memberData.Name += line.at(cursor);
			--cursor;
		}
		GetReflectNameAndReflectValueTypeAndReflectModifer(line, memberData.Name, memberData.ReflectValueType, memberData.ReflectModifier);

		cursor = (uint32_t)line.size() - 1;
		while (std::find(emptyChars.begin(), emptyChars.end(), line.at(cursor)) != emptyChars.end())
			--cursor;
		std::string type = line.substr(0, cursor + 1);

		// Make sure there are no empty chars in the type string.
		for (const char& c : emptyChars)
			Util::RemoveCharAll(type, c);
		memberData.Type = type;

		fileData.Cursor = endOfLineCursor;
		return memberData;
	}

	void FileParser::SkipFunctionBody(FileParsedData& fileData)
	{
		FileParsedData bracketCursor = fileData;
		FindNextChar(bracketCursor, '{');
		FileParsedData semicolonCursor = fileData;
		FindNextChar(semicolonCursor, ';');

		if (semicolonCursor.Cursor < bracketCursor.Cursor)
		{
			fileData.Cursor = semicolonCursor.Cursor;
			return;
		}
		fileData.Cursor = bracketCursor.Cursor;

		std::stack<char> brackets;
		char c = fileData.Data[fileData.Cursor];
		while (true)
		{
			if (c == '{')
				brackets.push('{');
			else if (c == '}')
				brackets.pop();

			if (brackets.size() == 0)
			{
				break;
			}
			c = fileData.Data[++fileData.Cursor];
		}
	}

	EReflectType FileParser::CheckForReflectType(FileParsedData& data)
	{
		auto find_closest_char = [data, this](std::vector<char> const& chars_to_find)
		{
			int cursor = INT_MAX;
			for (size_t i = 0; i < chars_to_find.size(); ++i)
			{
				char charToFind = chars_to_find.at(i);
				FileParsedData copy = data;
				FindNextChar(copy, charToFind);
				cursor = std::min(copy.Cursor, cursor);
			}
			return cursor;
		};

		int member_cursor = find_closest_char(memberStartChars);
		int function_cursor = find_closest_char(functionStartChars);

		bool isTemplate = data.Data.find(TemplateKey, data.Cursor) < member_cursor;

		if (member_cursor < function_cursor && !isTemplate)
		{
			return EReflectType::Member;
		}
		else if (function_cursor != INT_MAX && !isTemplate)
		{
			return EReflectType::Function;
		}

		if (isTemplate)
		{
			SkipFunctionBody(data);
		}
		return EReflectType::Unknown;
	}

	bool FileParser::CheckForEndOfFile(FileParsedData& fileData, int cursor)
	{
		if (fileData.Cursor >= cursor)
		{
			return true;
		}

		FileParsedData copy = fileData;
		char c = copy.Data[copy.Cursor];
		bool endOfFile = false;
		char previousValidChar = c;
		while (copy.Cursor < cursor)
		{
			++copy.Cursor;
			c = copy.Data[copy.Cursor];
			if (std::find(emptyChars.begin(), emptyChars.end(), c) == emptyChars.end())
			{
				if (previousValidChar == '}')
				{
					endOfFile = c == ';';
				}
				break;
			}
		}
		if (endOfFile)
			fileData.Cursor = ++copy.Cursor;
		return endOfFile || copy.Cursor == cursor;
	}

	EReflectValueType FileParser::CheckForRefOrPtr(std::string_view view)
	{
		size_t referenceIndex = view.find(ReferenceKey);
		size_t pointerIndex = view.find(PointerKey);

		// Get the type. We need this as the code generation will need to add some casting 
		// if the type is not a value.
		if (referenceIndex == std::string::npos && pointerIndex == std::string::npos)
			return EReflectValueType::Value;
		else if (referenceIndex < pointerIndex)
			return EReflectValueType::Reference;
		else if (pointerIndex < referenceIndex && referenceIndex == std::string::npos)
			return EReflectValueType::Pointer;
		else if (pointerIndex < referenceIndex && referenceIndex != std::string::npos)
			return EReflectValueType::PointerReference;
		// TODO: Think about pointer, pointer, pointer, etc. 
		assert(false && "[FileParser::CheckForRefOrPtr] Unknow type.");
		return EReflectValueType::Value;
	}

	EReflectValueModifier FileParser::CheckForMemberModifers(std::string_view view)
	{
		size_t constIndex = view.find(ConstKey);
		size_t staticIndex = view.find(StaticKey);
		size_t volatileIndex = view.find(VolatileKey);
		size_t virtualIndex = view.find(VirtualKey);

		if (constIndex != std::string::npos)
			return EReflectValueModifier::Const;
		else if (staticIndex != std::string::npos)
			return EReflectValueModifier::Static;
		else if (volatileIndex != std::string::npos)
			return EReflectValueModifier::Volatile;
		else if (virtualIndex != std::string::npos)
			return EReflectValueModifier::Virtual;

		//TODO: Think about const static
		return EReflectValueModifier::None;
	}

	std::vector<ReflectTypeNameData> FileParser::ReflectGetFunctionParameters(std::string_view view)
	{
		int cursor = 0;
		if (view.at(0) == '(')
			++cursor;

		std::vector<ReflectTypeNameData> parameters;
		std::string str;
		char c = view.at(cursor);
		while (cursor < view.size())
		{
			if (c != ',' && c != ')')
				str += c;
			else
			{
				if (str.size() > 0 && !Util::StringContains(str, emptyChars))
				{
					ReflectTypeNameData parameter;
					// Extract the name, type for the parameter.
					int copyCursor = cursor - 1;
					while (std::find(emptyChars.begin(), emptyChars.end(), view.at(copyCursor)) != emptyChars.end())
						--copyCursor;

					while (std::find(emptyChars.begin(), emptyChars.end(), view.at(copyCursor)) == emptyChars.end())
					{
						parameter.Name += view.at(copyCursor);
						--copyCursor;
					}
					GetReflectNameAndReflectValueTypeAndReflectModifer(str, parameter.Name, parameter.ReflectValueType, parameter.ReflectModifier);

					// Make sure there are no empty chars in the type string.
					for (const char& emptyChar : emptyChars)
						Util::RemoveCharAll(str, emptyChar);
					parameter.Type = str;

					parameters.push_back(parameter);
				}
				if (c == ')')
				{
					break;
				}
			}
			++cursor;
			c = view.at(cursor);
			//auto [type, name, isConst] = ReflectTypeAndName(fileData, { ',', ')' });
			//parameters.push_back(
			//	{
			//		type,
			//		name,
			//		DEFAULT_TYPE_SIZE,
			//		type.back() == '*' || type.back() == '&' ? (type.back() == '*' ? EReflectValueType::Pointer : EReflectValueType::Reference) : EReflectValueType::Value,
			//		isConst
			//	});
		}
		return parameters;
	}

#ifndef EXP_PARSER
	bool FileParser::RefectCheckForEndOfLine(const FileParsedData& fileData)
	{
		char c = fileData.Data[fileData.Cursor];
		if (c == ' ' || c == '(' || c == ';')
		{
			return true;
		}

		return false;
	}

	bool FileParser::ReflectTypeCheck(const std::string& type)
	{
		if (type == "const")
		{
			return true;
		}

		return false;
	}

	std::tuple<std::string, std::string, bool> FileParser::ReflectTypeAndName(FileParsedData& fileData, const std::vector<char>& endOfLineCharacters)
	{
		std::string type;
		bool typeFound = false;
		std::string name;
		bool nameFound = false;
		bool isConst = false;

		while (true)
		{
			char c = fileData.Data[fileData.Cursor];
			if (c == '}')
			{
				// Break here if we have finished
				break;
			}

			if (RefectCheckForEndOfLine(fileData) || (std::find(endOfLineCharacters.begin(), endOfLineCharacters.end(), c) != endOfLineCharacters.end()))
			{
				if (!typeFound)
				{
					if (!type.empty())
					{
						typeFound = true;
						CheckForConst(fileData, type, typeFound, isConst);
					}
				}
				else if (!nameFound)
				{
					if (!name.empty())
					{
						nameFound = true;
					}
				}
			}
			else if (c != '\n' && c != '\t') /*if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_')*/
			{
				if (!typeFound)
				{
					type += c;
				}
				else if (!nameFound)
				{
					name += c;
				}
			}

			if ((typeFound && nameFound))
			{
				break;
			}
			++fileData.Cursor;
		}
		return std::make_tuple<std::string, std::string>(type.c_str(), name.c_str(), isConst);
	}

	void FileParser::CheckForConst(FileParsedData& fileData, std::string& type, bool& typeFound, bool& isConst)
	{
		const int len = 6;
		std::string tmp;
		if (isConst)
		{
			return;
		}

		for (int i = fileData.Cursor - (len - 1); i < fileData.Cursor; ++i)
		{
			if (i < 0)
			{
				break;
			}
			tmp += fileData.Data[i];
		}
		if (tmp == "const")
		{
			type += ' ';
			typeFound = false;
			isConst = true;
			return;
		}

		tmp = "";
		for (int i = fileData.Cursor + 1; i < fileData.Cursor + len; ++i)
		{
			if (i > static_cast<int>(fileData.Data.size()))
			{
				break;
			}
			tmp += fileData.Data[i];
		}
		if (tmp == "const")
		{
			type += ' ' + tmp;
			fileData.Cursor += 5;
			typeFound = false;
			isConst = true;
		}
	}
#endif

	int FileParser::CountNumberOfSinceTop(const FileParsedData& fileData, int cursorStart, const char& character)
	{
		int count = 0;
		while (cursorStart > 0)
		{
			//TODO Out of bounds checks.
			if (fileData.Data[cursorStart] == character)
			{
				++count;
			}
			--cursorStart;
		}
		return count;
	}
}
