#pragma once

#include "Core/Core.h"
#include "Core/Enums.h"
#include "Core/Util.h"
#include <vector>
#include <functional>
#include <type_traits>
#include <istream>
#include <vector>

namespace Reflect
{
	struct IReflect;
	class Class;
	struct Constructor;

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
		std::vector<std::string> Flags;
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

	//
	// These exist if we ever need to provide some sort of common interface.
	//
	class ISerialiser { public: virtual ~ISerialiser() {} };
	class IUnserialiser { public: virtual ~IUnserialiser() {} };

	using ReadMemberType = void (*)(IUnserialiser& u, std::istream& in, void* self);
	using WriteMemberType = void (*)(ISerialiser& s, std::ostream& out, const void* self);

	struct ReflectMemberProp
	{
	public:
		REFLECT_CONSTEXPR ReflectMemberProp(const char* name, const std::string& type, int offset, std::vector<std::string> const& strProperties, const Class* staticClass, bool isPointer, const ReadMemberType& read, const WriteMemberType& write)
			: Name(name)
			, Type(type)
			, StaticClass(staticClass)
			, IsPointer(isPointer)
			, Offset(offset)
			, StrProperties(strProperties)
			, Read(read)
			, Write(write)
		{}

		REFLECT_CONSTEXPR ReflectMemberProp(const char* name, const std::string& type, int offset, std::vector<std::string> const& strProperties, const Enum& staticEnum, const ReadMemberType& read, const WriteMemberType& write)
			: Name(name)
			, Type(type)
			, StaticEnum(&staticEnum)
			, Offset(offset)
			, StrProperties(strProperties)
			, Read(read)
			, Write(write)
		{ }

		REFLECT_CONSTEXPR bool ContainsProperty(std::vector<std::string> const& flags) const
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

		REFLECT_CONSTEXPR bool GetPropertyValue(const std::string_view &flag, std::string& value) const
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

		const char* const		Name;
		const std::string		Type;
		const Class* const		StaticClass = nullptr;
		const Enum* const		StaticEnum = nullptr;
		const bool				IsPointer = false;
		const int				Offset;
		const std::vector<std::string> StrProperties;
		const ReadMemberType	Read;
		const WriteMemberType	Write;
	};

	template<typename T>
	inline REFLECT_CONSTEXPR typename std::enable_if<!std::is_base_of_v<BaseEnumContainer, T>, ReflectMemberProp>::type 
		CreateReflectMemberProp(const char* name, const std::string& type, int offset, std::vector<std::string> const& strProperties, const ReadMemberType& read, const WriteMemberType& write)
	{
		return ReflectMemberProp(name, type, offset, strProperties, Reflect::Util::GetStaticClass<T>(), std::is_pointer_v<T>, read, write);
	}

	template<typename T>
	inline REFLECT_CONSTEXPR typename std::enable_if<std::is_base_of_v<BaseEnumContainer, T>, ReflectMemberProp>::type 
		CreateReflectMemberProp(const char* name, const std::string& type, int offset, std::vector<std::string> const& strProperties, const ReadMemberType& read, const WriteMemberType& write)
	{
		return ReflectMemberProp(name, type, offset, strProperties, T::StaticEnum, read, write);
	}

	/// <summary>
	/// Store arguments for a function ptr
	/// </summary>
	struct FunctionPtrArgs
	{
	public:
		struct Arg
		{
			Arg(const std::string_view& type, void* ptr)
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
		ReflectMember(const ReflectMemberProp *prop, void* memberPtr)
			: Properties(prop)
			, RawPointer(memberPtr)
		{}

		const ReflectMemberProp* const	Properties;
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

	class REFLECT_DLL ClassAllocator
	{
		using AllocateType   = IReflect*(*)();
		using ConstructType  = void(*)(IReflect* obj, const Constructor& init);
		using DestroyType    = void(*)(IReflect* obj);
		using DeallocateType = void(*)(IReflect* obj);

		template<typename T> static IReflect* AllocateObject() { std::allocator<T> a; return a.allocate(1); }
		template<typename T> static void ConstructObject(IReflect* obj, const Constructor& init) { new((T*)obj) T(init); }
		template<typename T> static void DestroyObject(IReflect* obj) { ((T*)obj)->~T(); }
		template<typename T> static void DeallocateObject(IReflect* obj) { std::allocator<T> a; return a.deallocate((T*)obj, 1); }

		ClassAllocator(const AllocateType& allocate, const ConstructType& construct, const DestroyType& destroy, const DeallocateType& deallocate)
			: Allocate(allocate)
			, Construct(construct)
			, Destroy(destroy)
			, Deallocate(deallocate)
		{}

	public:
		const AllocateType   Allocate;
		const ConstructType  Construct;
		const DestroyType    Destroy;
		const DeallocateType Deallocate;

		template<typename T>
		static ClassAllocator Create()
		{
			return ClassAllocator(AllocateObject<T>, ConstructObject<T>, DestroyObject<T>, DeallocateObject<T>);
		}
	};

	class Class
	{
	public:
		Class(const char *name, const Class *super, std::vector<std::string> const& strProperties, size_t prop_count, const ReflectMemberProp *props, const ClassAllocator& allocator)
			: Name(name)
			, SuperClass(super)
			, StrProperties(strProperties)
			, m_member_prop_count(prop_count)
			, m_member_props(props)
			, Allocator(allocator)
		{
			Register(*this);
		}

		~Class()
		{
			Unregister(*this);
		}

		// Makes the type usable by Allocator.
		REFLECT_DLL static void Register(Class& c);
		REFLECT_DLL static void Unregister(Class& c);

		// Map a type name to a different type.
		REFLECT_DLL static void RegisterOverride(const char *name, const Class& c);

		// Reflect!
		REFLECT_DLL static Class* Lookup(const std::string_view &name);
		REFLECT_DLL static std::vector<std::reference_wrapper<Class>> LookupWhere(const std::function<bool(const Class&)>& pred);

		REFLECT_DLL static std::vector<std::reference_wrapper<Class>> LookupDescendantsOf(const Class& c);
		template<typename T> static auto LookupDescendantsOf() { return LookupDescendantsOf(T::StaticClass); }

		template<typename T>
		bool IsOrDescendantOf() const { return IsOrDescendantOf(T::StaticClass); }
		inline bool IsOrDescendantOf(const Class& c) const 
		{
			return 
				this == &c || 
				(SuperClass ? SuperClass->IsOrDescendantOf(c) : false);
		}

		REFLECT_DLL std::vector<ReflectMember> GetMembers(std::vector<std::string> const& flags, bool recursive=true) const
		{
			std::vector<Reflect::ReflectMember> members;

			GetMembersInternal(members, flags, recursive);
			
			return members;
		}

		bool ContainsProperty(std::vector<std::string> const& flags) const
		{
			return Util::ContainsProperty(StrProperties, flags);
		}

		bool GetPropertyValue(const std::string_view &flag, std::string_view& value) const
		{
			return Util::GetPropertyValue(StrProperties, flag, value);
		}

		const char* const				Name;
		const Class* const				SuperClass;
		const ClassAllocator			Allocator;
		const std::vector<std::string>	StrProperties;

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
					members.push_back(Reflect::ReflectMember(&member, (void *)(size_t)member.Offset));
				}
			}
		}

		const size_t					m_member_prop_count;
		const ReflectMemberProp* const	m_member_props;
	};

	struct Constructor
	{
		Constructor(const Class& type, IReflect* outer = nullptr, uint32_t flags = 0) : Type(type), Outer(outer), Flags(flags) {}
		const Class& Type;
		IReflect* const Outer;
		const uint32_t Flags;
	};

	struct REFLECT_DLL IReflect
	{
		// Initialisation.
		IReflect(const Constructor& init) : m_class(&init.Type) {}
		virtual ~IReflect() {}
		
		// Misc.
		const auto& GetClass() const { return *m_class; }

		// Reflection.
		virtual ReflectFunction GetFunction(const std::string_view& functionName) const { (void)functionName; return ReflectFunction(nullptr, nullptr);};
		virtual ReflectMember GetMember(const std::string_view& memberName) const { (void)memberName; return ReflectMember(nullptr, nullptr); };
		virtual std::vector<ReflectMember> GetMembers(std::vector<std::string> const& flags) const { (void)flags; return {}; };
		virtual std::vector<ReflectMember> GetMembers() const { return {}; };
		
		// Serialisation.
		virtual void PostUnserialise() {}

		// Cleanup.
		virtual void Dispose() noexcept {}							// Kick off the destruction of threaded resources.
																	// We don't want to block the job thread waiting for a destruction.
		virtual bool IsDisposeComplete() noexcept { return true; }	// Whether the threaded resources are freed.
		virtual void Finalise() noexcept {}							// Final cleanup of internal resources.

	private:
		// Don't mark this as 'const Class* const'! Prevents the assignment operator from working.
		const Class* m_class;
	};
}

#define REFLECT_BASE() public Reflect::IReflect
#define REFLECT_STRUCT(...)
#define REFLECT_CLASS(...)
#define REFLECT_ENUM(...)
