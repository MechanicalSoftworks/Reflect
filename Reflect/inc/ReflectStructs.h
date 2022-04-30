#pragma once

#include "Core/Core.h"
#include "Core/Enums.h"
#include "Core/Allocator.h"
#include "Core/Util.h"
#include <vector>
#include <functional>
#include <type_traits>
#include <istream>
#include <vector>

namespace Reflect
{
	class Serialiser;
	class Unserialiser;
	struct IReflect;
	class Class;
	class Enum;
	struct Initialiser;
	struct UnserialiseField;

	struct ReflectTypeNameData
	{
		std::string Type;
		std::string Name;
		int TypeSize;
		Reflect::ReflectMemberType ReflectMemberType;
		bool IsConst;
		std::vector<std::string> ContainerProps;

		ReflectTypeNameData()
			: Type("Unkown")
			, Name("Unkown")
			, TypeSize(0)
			, ReflectMemberType(ReflectMemberType::Value)
			, IsConst(false)
		{ }

		ReflectTypeNameData(const std::string& type, const std::string& name, const int& typeSize, const Reflect::ReflectMemberType& memberType, const bool& isConst)
			: Type(type)
			, Name(name)
			, TypeSize(typeSize)
			, ReflectMemberType(memberType)
			, IsConst(isConst)
		{ }

		bool operator!=(const ReflectTypeNameData& other) const
		{
			bool propsEqual = ContainerProps.size() != other.ContainerProps.size();
			if (propsEqual)
			{
				for (uint32_t i = 0; i < ContainerProps.size(); ++i)
				{
					if (ContainerProps[i] == other.ContainerProps[i])
					{
						propsEqual = false;
						break;
					}
				}
			}
			return Type != other.Type ||
				Name != other.Name ||
				TypeSize != other.TypeSize ||
				propsEqual;
		}
	};

	struct ReflectMemberData : public ReflectTypeNameData
	{
		Reflect::ReflectType ReflectType = ReflectType::Member;
	};

	struct ReflectFunctionData : public ReflectTypeNameData
	{
		Reflect::ReflectType ReflectType = ReflectType::Function;
		std::vector<ReflectTypeNameData> Parameters;
	};

	struct ReflectConstantData
	{
		std::string Name;
		int64_t Value;
	};

	struct ReflectContainerData : public ReflectTypeNameData
	{
		std::string Name, SuperName;
		Reflect::ReflectType ReflectType;
		int ReflectGenerateBodyLine;

		std::vector<ReflectConstantData> Constants;
		std::vector<ReflectMemberData> Members;
		std::vector<ReflectFunctionData> Functions;
	};

	struct FileParsedData
	{
		std::string Data;
		int Cursor;
		std::string FilePath;
		std::string RelativeFilePath;
		std::string SubPath;
		std::string FileName;
		std::string FileExtension;
		int GeneratedBodyLineOffset;

		std::vector<ReflectContainerData> ReflectData;
	};

	struct ReflectMemberProp
	{
	public:
		ReflectMemberProp(const char* name, const std::string &type, int offset, std::vector<std::string> const& strProperties, const Class* staticClass, bool isPointer)
			: Name(name)
			, Type(type)
			, StaticClass(staticClass)
			, IsPointer(isPointer)
			, Offset(offset)
			, StrProperties(strProperties)
		{ }

		ReflectMemberProp(const char* name, const std::string& type, int offset, std::vector<std::string> const& strProperties, std::unique_ptr<Enum>&& staticEnum)
			: Name(name)
			, Type(type)
			, StaticEnum(std::move(staticEnum))
			, Offset(offset)
			, StrProperties(strProperties)
		{ }

		bool ContainsProperty(std::vector<std::string> const& flags) const
		{
			for (auto const& flag : flags)
			{
				for (auto const& p : StrProperties)
				{
					if (p == flag || (p.length() >= flag.length() && p.find(flag) == 0 && p[flag.length()] == '='))
					{
						return true;
					}
				}
			}
			return false;
		}

		bool GetPropertyValue(const std::string_view &flag, std::string& value) const
		{
			for (auto const& p : StrProperties)
			{
				if (p.find(flag) != 0)
				{
					continue;
				}

				const auto assign = p.find('=');
				if (assign != flag.length())
				{
					continue;
				}

				value = p.substr(assign + 1);
				return true;
			}

			return false;
		}

		const char* const	Name;
		const std::string	Type;
		const Class* const	StaticClass = nullptr;
		const bool			IsPointer = false;
		const std::unique_ptr<Enum>	StaticEnum;
		const int			Offset;
		const std::vector<std::string> StrProperties;
	};

	template<typename T> inline const std::vector<std::pair<std::string, T>>& EnumValues();
	template<typename T> inline const std::map<std::string, T>& EnumMap();

	template<typename T>
	inline typename std::enable_if<!std::is_enum_v<T>, ReflectMemberProp>::type CreateReflectMemberProp(const char* name, const std::string& type, int offset, std::vector<std::string> const& strProperties)
	{
		return ReflectMemberProp(name, type, offset, strProperties, Reflect::Util::GetStaticClass<T>(), std::is_pointer_v<T>);
	}

	template<typename T>
	inline typename std::enable_if<std::is_enum_v<T>, ReflectMemberProp>::type CreateReflectMemberProp(const char* name, const std::string& type, int offset, std::vector<std::string> const& strProperties)
	{
		class EnumImplementation final : public Reflect::Enum
		{
			static auto GenerateMap()
			{
				std::map<std::string, int> int_values;
				for (const auto& it : EnumValues<T>())
				{
					int_values.insert(std::pair<std::string, int>(it.first, (int)it.second));
				}
				return int_values;
			}

			static auto GenerateValues()
			{
				std::vector<std::pair<std::string, int>> int_values;
				for (const auto& it : EnumValues<T>())
				{
					int_values.push_back(std::pair<std::string, int>(it.first, (int)it.second));
				}
				return int_values;
			}

		public:
			EnumImplementation()
				: m_map(GenerateMap())
				, m_values(GenerateValues())
			{}

			const char* ToString(int v) const final override
			{
				const char* EnumToString(T);
				return EnumToString((T)v);
			}

			const std::map<std::string, int>& Map() const final override { return m_map; }
			const std::vector<std::pair<std::string, int>>& Values() const final override { return m_values; }

			int Parse(const std::string& str) const final override
			{
				bool StringToEnum(const std::string&, T&);

				T v = (T)0;
				if (!StringToEnum(str, v))
				{
					throw new std::runtime_error("Unknown string");
				}

				return (int)v;
			}

			bool ParseTo(const std::string& str, void* ptr) const final override
			{
				bool StringToEnum(const std::string&, T&);
				return StringToEnum(str, *(T*)ptr);
			}

			int IndexOf(const void* ptr) const final override
			{
				const auto& val = *(T*)ptr;
				for (size_t i = 0; i < m_values.size(); ++i)
				{
					if ((T)m_values[i].second == val)
					{
						return (int)i;
					}
				}
				return -1;
			}

			void AssignIndex(void* ptr, int index) const final override
			{
				*(T*)ptr = (T)m_values.at(index).second;
			}

		private:
			const std::map<std::string, int> m_map;
			const std::vector<std::pair<std::string, int>> m_values;
		};

		auto e = std::make_unique<EnumImplementation>();
		return ReflectMemberProp(name, type, offset, strProperties, std::move(e));
	}

	/// <summary>
	/// Store arguments for a function ptr
	/// </summary>
	struct FunctionPtrArgs
	{
	public:
		struct Arg
		{
			Arg(std::string type, void* ptr)
				: Type(type)
				, Ptr(ptr)
			{ }

			void* Get() const { return Ptr; }

		private:
			std::string Type;
			void* Ptr;
		};

		FunctionPtrArgs() { }
		FunctionPtrArgs(const std::vector<Arg>& args)
			: m_args(args)
		{ }

		void* GetArg(int index)
		{
			return m_args.at(index).Get();
		}

		template<typename T>
		REFLECT_DLL void AddArg(T* obj)
		{
			m_args.push_back(Arg(Reflect::Util::GetTypeName(*obj), obj));
		}

	private:
		std::vector<Arg> m_args;
	};

	using FunctionPtr = Reflect::ReflectReturnCode(*)(void* objectPtr, void* returnValue, FunctionPtrArgs& args);

	struct ReflectFunction
	{
		ReflectFunction(void* objectPtr, FunctionPtr func)
			: m_objectPtr(objectPtr)
			, m_func(func)
		{ }

		//template<typename... Args>
		//void Invoke(void* returnValue, Args... args)
		//{
		//	FunctionPtrArgs funcArgs = PackFunctionArgs(std::forward<Args>(args)...);
		//	int i = *static_cast<int*>(funcArgs.GetArg(0));
		//	int* ip = static_cast<int*>(funcArgs.GetArg(1));
		//	return (*Func)(ObjectPtr, returnValue, funcArgs);
		//}

		REFLECT_DLL Reflect::ReflectReturnCode Invoke(void* returnValue = nullptr, FunctionPtrArgs functionArgs = FunctionPtrArgs())
		{
			if (IsValid())
			{
				(*m_func)(m_objectPtr, returnValue, functionArgs);
				return ReflectReturnCode::SUCCESS;
			}
			return ReflectReturnCode::INVALID_FUNCTION_POINTER;
		}

		REFLECT_DLL bool IsValid() const
		{
			return m_objectPtr != nullptr;
		}

	private:
		template<typename... Args>
		FunctionPtrArgs PackFunctionArgs(Args... args)
		{
			std::vector<FunctionPtrArgs::Arg> funcArgs = { PackFunctionArg(args)... };
			return  FunctionPtrArgs(funcArgs);
		}

		template<typename T, typename... Args>
		FunctionPtrArgs::Arg PackFunctionArg(T& t, Args&... args)
		{
			return FunctionPtrArgs::Arg(Reflect::Util::GetTypeName(t), &t);
		}

		template<typename T, typename... Args>
		FunctionPtrArgs::Arg PackFunctionArg(T* t, Args... args)
		{
			return FunctionPtrArgs::Arg(Reflect::Util::GetTypeName(t), static_cast<void*>(t));
		}

	private:
		void* m_objectPtr;
		FunctionPtr m_func;
	};

	struct ReflectMember
	{
		ReflectMember(const ReflectMemberProp *prop, const UnserialiseField* unserialise, void* memberPtr)
			: Properties(prop)
			, Unserialise(unserialise)
			, RawPointer(memberPtr)
		{}

		const ReflectMemberProp* const	Properties;
		const UnserialiseField* const	Unserialise;
		void* const						RawPointer;

		bool IsValid() const { return RawPointer != nullptr; }
		auto GetName() const { return Properties->Name; }
		auto GetTypeName() const { return Properties->Type; }

		template<typename T>
		REFLECT_DLL T* ConvertToType()
		{
			const auto convertType = Reflect::Util::GetTypeName<T>();
			if (convertType != GetTypeName())
			{
				return nullptr;
			}
			return static_cast<T*>(RawPointer);
		}
	};

	//
	// TODO: Unify these thing a single Allocator class. No need for the global Allocator.
	//
	using AllocatorType = IReflect*(*)();
	template<typename T> IReflect* AllocateObject() { std::allocator<T> a; return a.allocate(1); }

	using ConstructorType = void(*)(IReflect* obj, const Initialiser& init);
	template<typename T> void PlacementNew(IReflect* obj, const Initialiser& init) { new((T*)obj) T(init); }

	using DestructorType  = void(*)(IReflect* obj);
	template<typename T> void PlacementDelete(IReflect* obj) { ((T*)obj)->~T(); }

	using FreeType = void(*)(IReflect* obj);
	template<typename T> void FreeObject(IReflect* obj) { std::allocator<T> a; return a.deallocate((T*)obj, 1); }

	// Helpers for reading individual fields.
	using ReadFieldType = void (*)(Unserialiser& u, std::istream& in, void* self);
	using WriteFieldType = void (*)(Serialiser& s, std::ostream& out, const void* self);
	struct UnserialiseField
	{
		UnserialiseField(const char* n, const std::string& t, ReadFieldType r, WriteFieldType w) : Name(n), Type(t), Read(r), Write(w) {}
		const char* const		Name;
		const std::string		Type;
		const ReadFieldType		Read;
		const WriteFieldType	Write;
	};

	class Enum
	{
	public:
		~Enum() {}

		virtual const char* ToString(int v) const = 0;

		virtual int Parse(const std::string& str) const = 0;
		virtual bool ParseTo(const std::string& str, void* ptr) const = 0;

		virtual const std::map<std::string, int>& Map() const = 0;
		virtual const std::vector<std::pair<std::string, int>>& Values() const = 0;

		virtual int IndexOf(const void* ptr) const = 0;
		virtual void AssignIndex(void* ptr, int index) const = 0;
	};

	class Class
	{
	public:
		Class(const char *name, const Class *super, size_t prop_count, const ReflectMemberProp *props, size_t unserialise_count, const UnserialiseField* unserialisers, AllocatorType allocate, ConstructorType constructor, DestructorType destructor, FreeType free_)
			: Name(name)
			, SuperClass(super)
			, m_member_prop_count(prop_count)
			, m_member_props(props)
			, m_unserialise_count(unserialise_count)
			, m_unserialise(unserialisers)
			, Allocate(allocate)
			, Constructor(constructor)
			, Destructor(destructor)
			, Free(free_)
		{
			Register(this);
		}

		~Class()
		{
			Unregister(this);
		}

		// Decoupled Allocate from Constructor because maybe users will want to do some memory reuse.
		const AllocatorType Allocate;
		const ConstructorType Constructor;
		const DestructorType Destructor;
		const FreeType Free;

		// Makes the type usable by Allocator.
		REFLECT_DLL static void Register(Class* c);
		REFLECT_DLL static void Unregister(Class* c);

		// Map a type name to a different type.
		REFLECT_DLL static void RegisterOverride(const char *name, const Class* c);

		// Reflect!
		REFLECT_DLL static Class* Lookup(const std::string_view &name);
		REFLECT_DLL static std::vector<Class*> DescendantsOf(const Class* c);
		
		template<typename T>
		static std::vector<Class*> DescendantsOf() { return DescendantsOf(&T::StaticClass); }

		template<typename T>
		bool IsOrDescendantOf() const { return IsOrDescendantOf(&T::StaticClass); }
		inline bool IsOrDescendantOf(const Class* c) const 
		{
			return 
				this == c || 
				(SuperClass ? SuperClass->IsOrDescendantOf(c) : false);
		}

		REFLECT_DLL std::vector<ReflectMember> GetMembers(std::vector<std::string> const& flags, bool recursive=true) const
		{
			std::vector<Reflect::ReflectMember> members;

			GetMembersInternal(members, flags, recursive);
			
			return members;
		}

		const char* const	Name;
		const Class* const	SuperClass;

	private:
		REFLECT_DLL void GetMembersInternal(std::vector<Reflect::ReflectMember>& members, std::vector<std::string> const& flags, bool recursive) const
		{
			if (recursive && SuperClass)
				SuperClass->GetMembersInternal(members, flags, recursive);

			for (size_t i = 0; i < m_member_prop_count; i++)
			{
				const auto& member = m_member_props[i];
				if (member.ContainsProperty(flags))
				{
					// Find an unserialiser.
					const UnserialiseField* unserialise = nullptr;
					for (size_t i = 0; i < m_unserialise_count; i++)
					{
						if (m_unserialise[i].Name == member.Name)
						{
							unserialise = m_unserialise + i;
							break;
						}
					}


					members.push_back(Reflect::ReflectMember(&member, unserialise, (void *)(size_t)member.Offset));
				}
			}
		}

		const size_t					m_member_prop_count;
		const ReflectMemberProp* const	m_member_props;

		const size_t					m_unserialise_count;
		const UnserialiseField* const	m_unserialise;
	};

	struct Initialiser
	{
		Initialiser(const Class* static_class, IReflect* outer)
			: Type(static_class)
			, Outer(outer)
		{}

		const Class* const Type;
		IReflect* const Outer;
	};

	struct REFLECT_DLL IReflect
	{
		// Initialisation.
		IReflect(const Initialiser& init) { m_class = init.Type; m_outer = init.Outer; }

		// Misc.
		const Class* GetClass() const { return m_class; }
		IReflect* GetOuter() const { return m_outer; }
		void SetOuter(IReflect* outer) { m_outer = outer; }

		// Reflection.
		virtual ReflectFunction GetFunction(const std::string_view& functionName) { (void)functionName; return ReflectFunction(nullptr, nullptr);};
		virtual ReflectMember GetMember(const std::string_view& memberName) { (void)memberName; return ReflectMember(nullptr, nullptr, nullptr); };
		virtual std::vector<ReflectMember> GetMembers(std::vector<std::string> const& flags) { (void)flags; return {}; };
		
		// Serialisation.
		virtual void Serialise(Serialiser &s, std::ostream& out) const { (void)s; (void)out; }
		virtual void Unserialise(Unserialiser &u, std::istream& in) { (void)u; (void)in; }
		virtual void PostUnserialise() {}

	private:
		const Class* m_class = nullptr;
		IReflect* m_outer = nullptr;
	};
}

#define REFLECT_BASE() public Reflect::IReflect
#define REFLECT_STRUCT(...)
#define REFLECT_CLASS(...)
#define REFLECT_ENUM(...)
