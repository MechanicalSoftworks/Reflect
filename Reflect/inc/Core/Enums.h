#pragma once

#include "Compiler.h"
#include "Util.h"
#include <string>
#include <vector>

namespace Reflect 
{
	enum ReflectFlags
	{
		Invalid = 1 << 0,

		Private = 1 << 1,
		Public = 1 << 2,
		Friend = 1 << 3,
	};
	REFLECT_DLL ReflectFlags StringToReflectFlags(const std::string& str);

	enum class ReflectType
	{
		Class,
		Struct,
		Member,
		Function,
		Enum,

		Count
	};

	enum class ReflectMemberType
	{
		Value,
		Reference,
		Pointer,

		Count
	};

	enum class ReflectReturnCode
	{
		SUCCESS,
		FAILED,

		CAST_FAILED,
		INVALID_FUNCTION_POINTER,
		INVALID_MEMBER,
	};
	REFLECT_DLL std::string ReflectReturnCodeToString(const ReflectReturnCode& code);

	struct REFLECT_DLL EnumConstant
	{
		REFLECT_CONSTEXPR EnumConstant(const std::string_view& name, int64_t value, const std::string_view& label, const std::vector<std::string>& flags)
			: Name(name)
			, Value(value)
			, DisplayLabel(label)
			, Flags(flags)
		{}

		bool ContainsProperty(std::vector<std::string> const& flags) const
		{
			return Util::ContainsProperty(Flags, flags);
		}

		std::string_view GetPropertyValue(const std::string_view& flag) const;

		auto TryGetPropertyValue(const std::string_view& flag, std::string_view& value) const
		{
			return Util::TryGetPropertyValue(Flags, flag, value);
		}
			
		const std::string_view				Name;
		const int64_t						Value;
		const std::string_view				DisplayLabel;
		const std::vector<std::string>		Flags;
	};

	class REFLECT_DLL Enum
	{
	public:
		using ConstantType = int64_t;

		using ValuesContainerType			= std::vector<Reflect::EnumConstant>;
		using StringToConstantContainerType	= std::map<std::string_view, ValuesContainerType::const_iterator>;
		using ValueToConstantContainerType	= std::map<ConstantType, ValuesContainerType::const_iterator>;
		
		using LoadFuncType	= ConstantType(*)(const void*);
		using StoreFuncType	= void(*)(void*, ConstantType);

		Enum(const std::string_view& name, const std::string& value_type_name, std::vector<std::string>&& strProperties, std::vector<Reflect::EnumConstant>&& values, const LoadFuncType& load, const StoreFuncType& store)
			: Name(name)
			, ValueTypeName(value_type_name)
			, StrProperties(std::move(strProperties))
			, Values(std::move(values))
			, StringToConstant(BuildStringToConstant(Values))
			, ValueToConstant(BuildValueToConstant(Values))
			, Load(load)
			, Store(store)
		{}

		bool ContainsProperty(std::vector<std::string> const& flags) const
		{
			return Util::ContainsProperty(StrProperties, flags);
		}

		std::string_view GetPropertyValue(const std::string_view& flag) const;

		bool TryGetPropertyValue(const std::string_view& flag, std::string_view& value) const
		{
			return Util::TryGetPropertyValue(StrProperties, flag, value);
		}

		const auto& GetConstant(ConstantType value) const
		{
			return *ValueToConstant.at(value);
		}

		bool TryParseTo(const std::string_view& str, void* ptr) const
		{
			const auto it = StringToConstant.find(str);
			if (it == StringToConstant.end())
			{
				return false;
			}

			Store(ptr, it->second->Value);
			return true;
		}

		int IndexOf(const void* ptr) const
		{
			const auto val = Load(ptr);
			for (size_t i = 0; i < Values.size(); ++i)
			{
				if (Values[i].Value == val)
				{
					return (int)i;
				}
			}
			return -1;
		}

		void AssignIndex(void* ptr, int index) const
		{
			Store(ptr, Values.at(index).Value);
		}

		const std::string_view& ToString(ConstantType v) const;
		const ConstantType& Parse(const std::string_view& v) const;

		template<typename T>
		bool TryParse(const std::string_view& value, T& v)
		{
			const auto it = StringToConstant.find(value);
			if (it == StringToConstant.end())
			{
				return false;
			}
			
			v = (T)it->second->Value;
			return true;
		}

		uint32_t ParseBitfieldString(const std::string_view& values) const
		{
			uint32_t  v = 0;

			for (const auto& it : Util::SplitStringView(values, '|'))
			{
				v |= Parse(Util::trim_stringview(it));
			}

			return v;
		}

		const std::string_view					Name;
		const std::string						ValueTypeName;
		const ValuesContainerType				Values;
		const std::vector<std::string>			StrProperties;

	private:
		const StringToConstantContainerType		StringToConstant;
		const ValueToConstantContainerType		ValueToConstant;

		const LoadFuncType		Load;
		const StoreFuncType		Store;

		static StringToConstantContainerType BuildStringToConstant(const ValuesContainerType& values)
		{
			StringToConstantContainerType lut;

			for (auto it = values.cbegin(); it != values.cend(); ++it)
			{
				lut.try_emplace(it->Name, it);
			}

			return lut;
		}

		static ValueToConstantContainerType BuildValueToConstant(const ValuesContainerType& values)
		{
			ValueToConstantContainerType lut;

			for (auto it = values.cbegin(); it != values.cend(); ++it)
			{
				lut.try_emplace(it->Value, it);
			}

			return lut;
		}
	};

	// This is for the benefit of 'CreateReflectMemberProp' who needs to determine whether something is an enum or a class.
	// !!! NOTE: It's very important this doesn't have any members, or virtual methods !!!
	// !!!       We pass these enum classes by value, like real enums.
	// !!!       Adding members adds function call overhead, which is a very bad thing!
	class IEnum {};

	template<class T> using EnableIfIEnum = std::enable_if_t<std::is_base_of_v<IEnum, T>>;
	template<class T> using EnableIfNotIEnum = std::enable_if_t<!std::is_base_of_v<IEnum, T>>;
}

/// <summary>
/// Dirty way of getting the enum string value.
/// </summary>
/// <typeparam name="E"></typeparam>
/// <returns></returns>
template <typename E, E> REFLECT_DLL std::string EnumToString()
{
	std::string value = FUNC_SIG;
	int startIndex = static_cast<int>(value.find_last_of(',')) + 1;
	int endIndex = static_cast<int>(value.find_last_of('>'));
	value = value.substr(startIndex, endIndex - startIndex);
	return value;
}