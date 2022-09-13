#pragma once

#include "Reflect.h"
#include "Core/Allocator.h"

#include <map>
#include <stack>

//
// Message format is:
//	Header:
//		[schema for each object in the stream]
//		[string pool]
//	Payload:
//		Root Object
//		 |- [ Fields ]
//		 |- [ Subobjects ]
// 
// When reading an old message:
//	* Removed fields are ignored,
//	* New fields are left at their default value.
//

namespace Reflect
{
	struct IReflect;
	class Serialiser;
	class Unserialiser;

	template<typename T> void WriteField(Serialiser& s, std::ostream& out, const T& v);
	template<typename T, size_t offset> void ReadField(Unserialiser& u, std::istream& in, void* self);

	//
	// Handles string de-deplication.
	//
	class REFLECT_DLL StringPool
	{
		struct entry_t
		{
			entry_t(uint32_t off, uint32_t len) : offset(off), length(len) {}

			entry_t(Unserialiser& u, std::istream& fin);
			void operator()(Serialiser& s, std::ostream& fout) const;

			uint32_t		offset;
			uint32_t		length;
		};

	public:
		// 4 billion strings should be enough, right?
		using index_t = uint32_t;

		index_t Add(const std::string_view& view);
		const std::string_view At(index_t i) const;
		index_t Count() const { return (index_t)m_entries.size(); }

		void Clear() { m_entries.clear(); m_pool.clear(); m_cache.clear(); }

		void Serialise(Serialiser& s, std::ostream& fout) const;
		void Unserialise(Unserialiser& u, std::istream& fin);

	private:
		std::vector<entry_t>	m_entries;
		std::string				m_pool;

		// Only populated while building.
		std::map<std::string, index_t>	m_cache;
	};

	//
	// Information needed to parse the message on the other end.
	// This type is deliberated not recursive.
	// Super classes must create their own entries, instead of having 
	// multiple derived classes redefine the same fields shared with their supers.
	//
	class REFLECT_DLL FieldSchema
	{
	public:
		// Exists for Unserialiser.
		FieldSchema() {}

		FieldSchema(const Reflect::Class* static_class, StringPool& pool);
		FieldSchema(const Reflect::ReflectMember& member, StringPool& pool);

		std::string Name;
		std::string Type;
		std::vector<FieldSchema> Fields;
	};

	//
	// Schema for user defined plain-old-data.
	// Basically, we just need this info to skip the field if necessary.
	//
	class UserDataType
	{
	public:
		// Exists for Unserialiser.
		UserDataType() {}

		UserDataType(const std::string& name, size_t sz) : Name(name), Size(sz) {}

		std::string Name;
		uint64_t	Size;
	};

	class REFLECT_DLL ISerialiser
	{
	public:
		virtual ~ISerialiser() {}
	};

	class REFLECT_DLL IUnserialiser
	{
	public:
		virtual ~IUnserialiser() {}
	};

	//
	// Serialises all objects tagged with 'serialise'.
	// The root object writes itself and all its children into a 'temp' stream.
	// At the same time, the root object and all its children register their schemas 
	// add entries to the string pool.
	// 
	// After all that is done, the actual output file is opened. Then the header,
	// schemas, string pool, and binary data are written.
	//
	class REFLECT_DLL Serialiser : public ISerialiser
	{
	public:
		Serialiser();
		Serialiser(const Serialiser&) = delete;
		Serialiser(const Serialiser&&) = delete;
		~Serialiser();

		void Write(std::ostream& fout, const IReflect& root);
		void Write(std::ostream& fout, const ReflectMember& member);

		// Include schemas for each object.
		// Allows us to load older messages (such as saved project files).
		void AddSchema(const Reflect::Class& static_class);

		// Add schemas for custom data types.
		void AddUserDataType(const std::string& name, size_t sz);
		template<typename T> void AddUserDataType(size_t sz)
		{
			AddUserDataType(Util::GetTypeName<T>(), sz);
		}

		StringPool::index_t AddString(const std::string_view& view) { return m_string_pool.Add(view); }

	private:
		void WriteInternal(std::ostream& fout);

		std::map<std::string, FieldSchema> m_schemas;
		std::map<std::string, UserDataType> m_user_data_types;
		StringPool m_string_pool;

		char m_temp_path[512];
	};

	//
	// Parses a message and recreates the objects.
	//
	class REFLECT_DLL Unserialiser : public IUnserialiser
	{
	public:
		struct SchemaDifference
		{
			SchemaDifference(bool fatal, const std::string& diff) : Fatal(fatal), Diff(diff) {}

			const bool Fatal;
			const std::string Diff;
		};

		Unserialiser(const Unserialiser&) = delete;
		Unserialiser(const Unserialiser&&) = delete;
		Unserialiser() {}

		bool ParseHeader(std::istream& in);
		const auto& GetSchemaDifferences() const { return m_schema_differences; }
		void Read(std::istream& in, IReflect* object = nullptr, IReflect* outer = nullptr);
		void Read(std::istream& in, ReflectMember& member);

		auto Detach() { return std::move(m_root); }

		const StringPool& GetStringPool() const { return m_string_pool; }
		const FieldSchema& GetSchema(const std::string& name) const;
		const UserDataType* GetUserDataType(const std::string& name) const;

		// Must be called after ParseHeader!
		void RegisterSchemaAlias(const char* alias, const char* old_type);

		// Keep track of the last object being processed - used for setting the outer object.
		void PushCurrentObject(IReflect* obj) { m_current_object.push(obj); }
		IReflect* GetCurrentObject() const { return m_current_object.top(); }
		void PopCurrentObject() { m_current_object.pop(); }

	private:
		std::map<std::string, FieldSchema> m_schemas;
		std::map<std::string, UserDataType> m_user_data_types;
		StringPool m_string_pool;

		std::vector<Unserialiser::SchemaDifference>	m_schema_differences;

		Ref<IReflect> m_root;
		Reflect::Class* m_root_class = nullptr;

		std::stack<IReflect*> m_current_object;
	};

	//--------------------------------------------------------------------------
	// FieldSchema implementation.
	//--------------------------------------------------------------------------

	inline FieldSchema::FieldSchema(const Reflect::Class* static_class, StringPool& pool)
		: Type(static_class->Name)
	{
		// Yes, technically 'name' is empty here. But we need to ensure the string
		// pool has an empty string for deserialisation!
		pool.Add(Name);
		pool.Add(Type);

		for (const auto& f : static_class->GetMembers({ "Serialise" }, false))
		{
			Fields.push_back(FieldSchema(f, pool));
		}
	}

	inline FieldSchema::FieldSchema(const Reflect::ReflectMember& member, StringPool& pool)
		: Name(member.GetName())
		, Type(member.GetTypeName())
	{
		pool.Add(Name);
		pool.Add(Type);
	}
}