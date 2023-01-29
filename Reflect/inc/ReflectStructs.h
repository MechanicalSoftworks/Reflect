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
		std::vector<std::string> Interfaces;
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
	inline REFLECT_CONSTEXPR typename std::enable_if<!std::is_base_of_v<IEnum, T>, ReflectMemberProp>::type 
		CreateReflectMemberProp(const char* name, const std::string& type, int offset, std::vector<std::string> const& strProperties, const ReadMemberType& read, const WriteMemberType& write)
	{
		return ReflectMemberProp(name, type, offset, strProperties, Reflect::Util::GetStaticClass<T>(), std::is_pointer_v<T>, read, write);
	}

	template<typename T>
	inline REFLECT_CONSTEXPR typename std::enable_if<std::is_base_of_v<IEnum, T>, ReflectMemberProp>::type
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

	using FunctionPtr = ReflectReturnCode(*)(void* objectPtr, void* returnValue, FunctionPtrArgs& args);

	struct ReflectMemberFunction
	{
	public:
		REFLECT_CONSTEXPR ReflectMemberFunction(const char* name, const FunctionPtr& function)
			: Name(name)
			, Function(function)
		{}

		const char* const		Name;
		const FunctionPtr		Function;
	};

	struct ReflectFunction
	{
		REFLECT_CONSTEXPR ReflectFunction(void* objectPtr, FunctionPtr func)
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

		REFLECT_DLL REFLECT_CONSTEXPR bool IsValid() const
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
		REFLECT_CONSTEXPR ReflectMember(const ReflectMemberProp *prop, void* memberPtr)
			: Properties(prop)
			, RawPointer(memberPtr)
		{}

		const ReflectMemberProp* const	Properties;
		void* const						RawPointer;

		bool IsValid() const			{ return RawPointer != nullptr; }
		const auto& GetName() const		{ return Properties->Name; }
		const auto& GetTypeName() const	{ return Properties->Type; }

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

		REFLECT_CONSTEXPR ClassAllocator(const AllocateType& allocate, const ConstructType& construct, const DestroyType& destroy, const DeallocateType& deallocate)
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

		operator bool() const { return Allocate && Construct && Destroy && Deallocate; }

		template<typename T>
		static REFLECT_CONSTEXPR ClassAllocator Create()
		{
			return ClassAllocator(AllocateObject<T>, ConstructObject<T>, DestroyObject<T>, DeallocateObject<T>);
		}

		// !!! For some reason, Visual Studio 2022 has compile errors when this is made constexpr, but only for templated classes !!!
		template<typename T>
		static ClassAllocator Create(std::nullptr_t)
		{
			return ClassAllocator(nullptr, nullptr, nullptr, nullptr);
		}
	};

	class Class
	{
	public:
		REFLECT_CONSTEXPR Class(const std::string &name, const Class *super, const ClassAllocator& allocator, std::vector<std::string> const& strProperties, std::vector<ReflectMemberProp>&& props, std::vector<ReflectMemberFunction>&& funcs, std::vector<std::string>&& interfaces)
			: Name(name)
			, SuperClass(super)
			, Allocator(allocator)
			, StrProperties(strProperties)
			, MemberProperties(std::move(props))
			, MemberFunctions(std::move(funcs))
			, Interfaces(std::move(interfaces))
		{
		}

		// Map a type name to a different type.
		REFLECT_DLL static void RegisterOverride(const char *name, const Class& c);

		// Reflect!
		REFLECT_DLL static const Class* TryLookup(const std::string_view &name);
		REFLECT_DLL static const Class& Lookup(const std::string_view &name);
		REFLECT_DLL static std::vector<std::reference_wrapper<Class>> LookupWhere(const std::function<bool(const Class&)>& pred);

		REFLECT_DLL static std::vector<std::reference_wrapper<Class>> LookupDescendantsOf(const Class& c);
		template<typename T> static auto LookupDescendantsOf() { return LookupDescendantsOf(T::StaticClass); }

		template<typename T>
		REFLECT_CONSTEXPR bool IsOrDescendantOf() const { return IsOrDescendantOf(T::StaticClass); }
		REFLECT_CONSTEXPR inline bool IsOrDescendantOf(const Class& c) const
		{
			return 
				this == &c || 
				(SuperClass ? SuperClass->IsOrDescendantOf(c) : false);
		}

		REFLECT_DLL REFLECT_CONSTEXPR auto GetMember(std::string_view const& memberName, IReflect* instance = nullptr) const
		{
			for (const auto* c = this; c != nullptr; c = c->SuperClass)
			{
				for (const auto& member : MemberProperties)
				{
					if (member.Name == memberName)
					{
						return ReflectMember(&member, (void*)((char*)instance + (size_t)member.Offset));
					}
				}
			}

			return ReflectMember(nullptr, nullptr);
		}

		REFLECT_DLL REFLECT_CONSTEXPR auto GetFunction(std::string_view const& funcName, IReflect* instance = nullptr) const
		{
			for (const auto* c = this; c != nullptr; c = c->SuperClass)
			{
				for (const auto& func : MemberFunctions)
				{
					if (func.Name == funcName)
					{
						return ReflectFunction(instance, func.Function);
					}
				}
			}

			return ReflectFunction(nullptr, nullptr);
		}

		REFLECT_DLL REFLECT_CONSTEXPR auto GetMembers(std::vector<std::string> const& flags, IReflect* instance = nullptr) const
		{
			std::vector<Reflect::ReflectMember> members;

			GetMembersInternal(members, flags, instance);
			
			return members;
		}

		template<typename T>
		REFLECT_CONSTEXPR bool HasInterface() const
		{
			for (const auto* c = this; c != nullptr; c = c->SuperClass)
			{
				if (std::find(c->Interfaces.begin(), c->Interfaces.end(), Util::GetTypeName<T>()) != c->Interfaces.end())
				{
					return true;
				}
			}

			return false;
		}

		REFLECT_CONSTEXPR bool ContainsProperty(std::vector<std::string> const& flags) const
		{
			return Util::ContainsProperty(StrProperties, flags);
		}

		REFLECT_CONSTEXPR bool GetPropertyValue(const std::string_view &flag, std::string_view& value) const
		{
			return Util::TryGetPropertyValue(StrProperties, flag, value);
		}

		const std::string				Name;
		const Class* const				SuperClass;
		const ClassAllocator			Allocator;
		const std::vector<std::string>	StrProperties;
		const std::vector<std::string>	Interfaces;

	private:
		REFLECT_DLL REFLECT_CONSTEXPR void GetMembersInternal(std::vector<Reflect::ReflectMember>& members, std::vector<std::string> const& flags, IReflect* instance) const
		{
			if (SuperClass)
				SuperClass->GetMembersInternal(members, flags, instance);

			for (const auto& member : MemberProperties)
			{
				if (member.ContainsProperty(flags))
				{
					members.push_back(Reflect::ReflectMember(&member, (void *)((char*)instance + (size_t)member.Offset)));
				}
			}
		}

		const std::vector<ReflectMemberProp>		MemberProperties;
		const std::vector<ReflectMemberFunction>	MemberFunctions;
	};

	class REFLECT_DLL LinkClass
	{
	public:
		LinkClass(const Class& c);
		~LinkClass();

	private:
		const Class& m_class;
	};

	struct Constructor
	{
		Constructor(const Class& type, IReflect* outer = nullptr, uint64_t object_flags = 0, uint64_t allocation_flags = 0)
			: Type(type)
			, Outer(outer)
			, ObjectFlags(object_flags)
			, AllocationFlags(allocation_flags)
		{}

		const Class& 		Type;
		IReflect* const 	Outer;
		const uint64_t		ObjectFlags;
		const uint64_t		AllocationFlags;
	};

	struct REFLECT_DLL IReflect
	{
		// Initialisation.
		IReflect(const Constructor& init) : m_class(&init.Type) {}
		virtual ~IReflect() {}
		
		// Misc.
		const auto& GetClass() const { return *m_class; }

		// Reflection.
		auto GetFunction(const std::string_view& functionName) const	{ return m_class->GetFunction(functionName, const_cast<IReflect*>(this)); }
		auto GetMember(const std::string_view& memberName) const		{ return m_class->GetMember(memberName, const_cast<IReflect*>(this)); }
		auto GetMembers(std::vector<std::string> const& flags) const	{ return m_class->GetMembers(flags, const_cast<IReflect*>(this)); }
		
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
