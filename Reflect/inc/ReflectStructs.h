#pragma once

#include "Core/Core.h"
#include "Core/Enums.h"
#include <vector>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <istream>
#include <vector>

struct ReflectFunction;
struct ReflectMember;

namespace Reflect
{
	struct ReflectTypeNameData
	{
		std::string Type;
		std::string Name;
		int TypeSize;
		ReflectMemberType ReflectMemberType;
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
		ReflectType ReflectType = ReflectType::Member;
	};

	struct ReflectFunctionData : public ReflectTypeNameData
	{
		ReflectType ReflectType = ReflectType::Function;
		std::vector<ReflectTypeNameData> Parameters;
	};

	struct ReflectContainerData : public ReflectTypeNameData
	{
		std::string Name, SuperName;
		ReflectType ReflectType;
		int ReflectGenerateBodyLine;

		std::vector<ReflectMemberData> Members;
		std::vector<ReflectFunctionData> Functions;
	};

	struct FileParsedData
	{
		std::string Data;
		int Cursor;
		std::string FilePath;
		std::string FileName;
		std::string FileExtension;
		int GeneratedBodyLineOffset;

		std::vector<ReflectContainerData> ReflectData;
	};

	struct ReflectMemberProp
	{
		ReflectMemberProp(const char* name, const std::string &type, int offset, std::vector<std::string> const& strProperties)
			: Name(name)
			, Type(type)
			, Offset(offset)
			, StrProperties(strProperties)
		{ }

		bool ContainsProperty(std::vector<std::string> const& flags) const
		{
			for (auto const& flag : flags)
			{
				for (auto const& p : StrProperties)
				{
					if (p == flag)
					{
						return true;
					}
				}
			}
			return false;
		}

		const char* Name;
		std::string Type;
		int Offset;
		std::vector<std::string> StrProperties;
	};

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
		ReflectMember(const char* memberName, std::string memberType, void* memberPtr)
			: m_name(memberName)
			, m_type(memberType)
			, m_ptr(memberPtr)
		{}

		REFLECT_DLL bool IsValid() const
		{
			return m_ptr != nullptr;
		}

		void* GetRawPointer() { return m_ptr; }
		const void* GetRawPointer() const { return m_ptr; }

		std::string GetName() const { return m_name; }

		std::string GetTypeName() const { return m_type; }

		template<typename T>
		REFLECT_DLL T* ConvertToType()
		{
			const auto convertType = Reflect::Util::GetTypeName<T>();
			if (convertType != m_type)
			{
				return nullptr;
			}
			return static_cast<T*>(m_ptr);
		}

	private:
		const char* m_name;
		std::string m_type;
		void* m_ptr;
		int m_offset;
	};

	typedef void (*ConstructorType)(void* obj);
	typedef void (*DestructorType)(void* obj);
	template<typename T> void PlacementNew(void* obj) { T::__PlacementNew((T*)obj); }
	template<typename T> void PlacementDelete(void* obj) { T::__PlacementDelete((T*)obj); }

	class Class
	{
	public:
		Class(const char *name, size_t size, size_t alignment, const Class *super, size_t prop_count, const ReflectMemberProp *props, ConstructorType constructor, DestructorType destructor)
			: m_name(name)
			, m_size(size)
			, m_alignment(alignment)
			, m_super_class(super)
			, m_member_prop_count(prop_count)
			, m_member_props(props)
			, Constructor(constructor)
			, Destructor(destructor)
		{
			Register(this);
		}

		~Class()
		{
			Unregister(this);
		}

		const ConstructorType Constructor;
		const DestructorType Destructor;

		REFLECT_DLL static void Register(Class* c);
		REFLECT_DLL static Class* Lookup(const std::string_view &name);
		REFLECT_DLL static void Unregister(Class* c);

		REFLECT_DLL std::vector<ReflectMember> GetMembers(std::vector<std::string> const& flags, bool recursive=true) const
		{
			std::vector<Reflect::ReflectMember> members;

			GetMembersInternal(members, flags);
			
			return members;
		}

		const char* GetName() const { return m_name; }
		size_t GetRawSize() const { return m_size; }
		size_t GetAlignment() const { return m_alignment; }
		size_t GetSize() const { return (m_size + m_alignment - 1) - m_size % m_alignment; }
		const Class* GetSuperClass() const { return m_super_class; }

	private:
		REFLECT_DLL void GetMembersInternal(std::vector<Reflect::ReflectMember>& members, std::vector<std::string> const& flags) const
		{
			if (m_super_class)
				m_super_class->GetMembersInternal(members, flags);

			for (size_t i = 0; i < m_member_prop_count; i++)
			{
				const auto& member = m_member_props[i];
				if (member.ContainsProperty(flags))
				{
					members.push_back(Reflect::ReflectMember(member.Name, member.Type, (void *)(size_t)member.Offset));
				}
			}
		}

		const char* m_name;
		const size_t m_size, m_alignment;
		const Class* m_super_class;
		const size_t m_member_prop_count;
		const ReflectMemberProp* m_member_props;
	};

	class Serialiser;
	class Unserialiser;

	struct REFLECT_DLL IReflect
	{
		// Initialisation.
		void Initialise(const Class* static_class) { m_class = static_class; }

		// Misc.
		const Class* GetClass() const { return m_class; }

		// Reflection.
		virtual ReflectFunction GetFunction(const std::string_view& functionName) { (void)functionName; return ReflectFunction(nullptr, nullptr);};
		virtual ReflectMember GetMember(const std::string_view& memberName) { (void)memberName; return ReflectMember("", "void", nullptr); };
		virtual std::vector<ReflectMember> GetMembers(std::vector<std::string> const& flags) { (void)flags; return {}; };
		
		// Serialisation.
		virtual void Serialise(Serialiser &s, std::ostream& out) const {}
		virtual void Unserialise(Unserialiser &u, std::istream& in) {}

	private:
		const Class* m_class = nullptr;
	};
}

#define REFLECT_BASE() public Reflect::IReflect
#define REFLECT_STRUCT(...)
#define REFLECT_CLASS(...)
