#pragma once

#include "Compiler.h"
#include "Core.h"
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
		constexpr EnumConstant(const std::string_view& name, int64_t value, const std::string_view& label, const std::vector<std::string>& flags)
			: Name(name)
			, Value(value)
			, DisplayLabel(label)
			, Flags(flags)
		{}

		bool ContainsProperty(std::vector<std::string> const& flags) const
		{
			return Util::ContainsProperty(Flags, flags);
		}

		bool GetPropertyValue(const std::string_view& flag, std::string_view& value) const
		{
			return Util::GetPropertyValue(Flags, flag, value);
		}
			
		const std::string_view				Name;
		const int64_t						Value;
		const std::string_view				DisplayLabel;
		const std::vector<std::string>		Flags;
	};

	class Enum
	{
	public:
		using ConstantType = int64_t;
		
		using LoadFuncType = ConstantType(*)(const void*);
		using StoreFuncType = void(*)(void*, ConstantType);

		REFLECT_CONSTEXPR Enum(const std::string_view& name, std::vector<std::string> const& strProperties, const std::vector<Reflect::EnumConstant>& values, const std::map<std::string_view, ConstantType>& string_to_enum, const std::map<ConstantType, std::string_view>& enum_to_string, const LoadFuncType& load, const StoreFuncType& store)
			: Name(name)
			, StrProperties(strProperties)
			, Values(values)
			, StringToEnum(string_to_enum)
			, EnumToString(enum_to_string)
			, Load(load)
			, Store(store)
		{}

		bool ContainsProperty(std::vector<std::string> const& flags) const
		{
			return Util::ContainsProperty(StrProperties, flags);
		}

		bool GetPropertyValue(const std::string_view& flag, std::string_view& value) const
		{
			return Util::GetPropertyValue(StrProperties, flag, value);
		}

		bool TryParseTo(const std::string_view& str, void* ptr) const
		{
			const auto it = StringToEnum.find(str);
			if (it == StringToEnum.end())
			{
				return false;
			}

			Store(ptr, it->second);
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

		const auto& ToString(ConstantType v) const			{ return EnumToString.at(v); }
		const auto& Parse(const std::string_view& v) const	{ return StringToEnum.at(v); }

		template<typename T>
		bool TryParse(const std::string_view& value, T& v)
		{
			const auto it = StringToEnum.find(value);
			if (it == StringToEnum.end())
			{
				return false;
			}
			
			v = (T)it->second;
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

		const std::string_view							Name;
		const std::vector<Reflect::EnumConstant>		Values;
		const std::vector<std::string>					StrProperties;

	private:
		const std::map<std::string_view, ConstantType>	StringToEnum;
		const std::map<ConstantType, std::string_view>	EnumToString;

		const LoadFuncType&		Load;
		const StoreFuncType&	Store;
	};

	// Thisis for the benefit of 'CreateReflectMemberProp' who needs to determine whether something is an enum or a class.
	// Can't use std::is_base_of against the template class EnumContainer.
	class BaseEnumContainer
	{};

	template<typename TEnum, typename TValue>
	class EnumContainer : public BaseEnumContainer
	{
	public:
		using ValueType = TValue;

		constexpr EnumContainer() : Value(0) {}
		constexpr EnumContainer(TValue v) : Value(v) {}

		static const auto& ToString(TEnum v)			{ return TEnum::StaticEnum.ToString(v); }
		       const auto& ToString() const				{ return TEnum::StaticEnum.ToString(Value); }

		auto Parse(const std::string_view& value)		{ return (ValueType)const_cast<Enum&>(TEnum::StaticEnum).Parse(value); }
		auto TryParse(const std::string_view& value)	{ return const_cast<Enum&>(TEnum::StaticEnum).TryParse(value, Value); }

		std::string ToBitfieldString() const
		{
			std::string s;

			for (const auto& it : TEnum::StaticEnum.Values)
			{
				if (Value & it.Value)
				{
					s.reserve(s.length() + it.Name.length() + 1);

					if (s.length())
					{
						s += '|';
					}

					s += it.Name;
				}
			}

			return s;
		}

		static ValueType ParseBitfieldString(const std::string_view& values)	{ return (ValueType)TEnum::StaticEnum.ParseBitfieldString(values); }

		TValue	load() const		{ return Value; }
		void	store(TValue v)		{ Value = v; }

	protected:
		TValue Value;
	};
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