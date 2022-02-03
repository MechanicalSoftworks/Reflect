#pragma once

#include "Reflect.h"
#include "Core/Allocator.h"

#include <map>

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

	//
	// Handles string de-deplication.
	//
	class StringPool
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
		typedef uint32_t index_t;

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
	class FieldSchema
	{
	public:
		// Exists for Unserialiser.
		FieldSchema() {}

		FieldSchema(const Reflect::Class* static_class, StringPool& pool);
		FieldSchema(const Reflect::ReflectMember& member, StringPool& pool);

		std::string name, type;
		std::vector<FieldSchema> fields;
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

		UserDataType(const std::string& _name, size_t _sz) : name(_name), sz(_sz) {}

		std::string name;
		uint64_t sz;
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
	class REFLECT_DLL Serialiser
	{
	public:
		Serialiser() {}
		Serialiser(const Serialiser&) = delete;
		Serialiser(const Serialiser&&) = delete;

		void Write(std::ostream& fout, const IReflect& root);

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
		std::map<std::string, FieldSchema> m_schemas;
		std::map<std::string, UserDataType> m_user_data_types;
		StringPool m_string_pool;
	};

	//
	// Parses a message and recreates the objects.
	//
	class REFLECT_DLL Unserialiser
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
		void Read(std::istream& in);

		auto Detach() { return std::move(m_root); }

		const StringPool& GetStringPool() const { return m_string_pool; }
		const FieldSchema& GetSchema(const std::string& name) const;
		const UserDataType* GetUserDataType(const std::string& name) const;

		// Must be called after ParseHeader!
		void RegisterSchemaAlias(const char* alias, const char* old_type);

		// Keep track of the last object being processed - used for setting the outer object.
		IReflect* GetCurrentObject() const { return m_current_object; }
		void SetCurrentObject(IReflect* obj) { m_current_object = obj; }

	private:
		std::map<std::string, FieldSchema> m_schemas;
		std::map<std::string, UserDataType> m_user_data_types;
		StringPool m_string_pool;

		std::vector<Unserialiser::SchemaDifference>	m_schema_differences;

		Ref<IReflect> m_root;
		Reflect::Class* m_root_class = nullptr;

		IReflect* m_current_object = nullptr;
	};

	// Helpers for reading individual fields.
	typedef void (*ReadFieldType)(Unserialiser &u, std::istream& s, void* self);
	struct UnserialiseField
	{
		UnserialiseField(const char* n, const std::string&t, ReadFieldType r) : name(n), type(t), read(r) {}
		const char* name;
		const std::string type;
		ReadFieldType read;
	};

	//--------------------------------------------------------------------------
	// Field serialisation.
	//--------------------------------------------------------------------------
	void SkipField(Unserialiser& u, std::istream& in, const std::string_view& type);

	namespace FieldImpl
	{
		//----------------------------------------------------------------------
		// Misc templates (declarations).
		// These are used by the generic templates. Need to be known to the compiler.

		inline void write(Serialiser& s, std::ostream& out, const std::string& v);
		inline void read(Unserialiser& u, std::istream& in, std::string& v);
		inline void write(Serialiser& s, std::ostream& out, const std::string_view& v);
		inline void write(Serialiser& s, std::ostream& out, const StringPool& p);
		inline void read(Unserialiser& u, std::istream& in, StringPool& p);
		inline void write(Serialiser& s, std::ostream& out, const FieldSchema& f);
		inline void read(Unserialiser& u, std::istream& in, FieldSchema& f);
		inline void write(Serialiser& s, std::ostream& out, const UserDataType& t);
		inline void read(Unserialiser& u, std::istream& in, UserDataType& t);

		template<typename T>
		inline void write(Serialiser& s, std::ostream& out, const Ref<T>& r)
		{
			s.AddSchema(*r->GetClass());
			write(s, out, std::string_view(r->GetClass()->GetName()));
			r->Serialise(s, out);
		}

		template<typename T>
		inline void read(Unserialiser& u, std::istream& in, Ref<T>& r)
		{
			std::string type_name;
			read(u, in, type_name);

			r = Allocator::Create<T>(type_name, u.GetCurrentObject());
			r->Unserialise(u, in);
		}

		//----------------------------------------------------------------------
		// Generic templates.
		// 
		// Reads a field from the stream into the given member.
		// Disabled for IReflect types because they need to go through their own overload, below.

		//
		// Default types (non IReflect).
		// Since these use binary IO they're only enabled for integer & float types.
		// These serve as the blocks upon which all other read + write functions are built upon.
		//
		template<typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
		inline typename std::enable_if<!std::is_base_of<IReflect, T>::value>::type
			write(Serialiser& s, std::ostream& out, const T& v)
		{
			out.write((const char*)&v, sizeof(v));
		}

		template<typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
		inline typename std::enable_if<!std::is_base_of<IReflect, T>::value>::type
			read(Unserialiser& u, std::istream& in, T& v)
		{
			in.read((char*)&v, sizeof(v));
		}

		template<typename T>
		inline void skip(Unserialiser& u, std::istream& in)
		{
			T t;
			read(u, in, t);
		}

		//
		// Vector types (non IReflect).
		//
		template<typename T>
		inline typename std::enable_if<!std::is_base_of<IReflect, T>::value>::type
			write(Serialiser& s, std::ostream& out, const std::vector<T>& v)
		{
			write(s, out, (uint32_t)v.size());
			for (const auto &it : v)
			{
				write(s, out, it);
			}
		}

		template<typename T>
		inline typename std::enable_if<!std::is_base_of<IReflect, T>::value>::type
			read(Unserialiser& u, std::istream& in, std::vector<T>& v)
		{
			uint32_t count;
			read(u, in, count);

			v.reserve(count);

			for (uint32_t i = 0; i < count; i++)
			{
				T t;
				read(u, in, t);
				v.push_back(std::move(t));
			}
		}

		inline void skip_vector(Unserialiser& u, std::istream& in, const std::string_view& vector_type)
		{
			uint32_t count;
			read(u, in, count);

			const std::string_view vector_string("std::vector<");
			const std::string_view value_type(vector_type.data() + vector_string.length(), vector_type.length() - vector_string.length() - 1);	// -1 for tailing '>'.

			for (uint32_t i = 0; i < count; i++)
			{
				SkipField(u, in, value_type);
			}
		}

		//
		// Map types (non IReflect).
		//
		template<typename K, typename V>
		inline typename std::enable_if<!std::is_base_of<IReflect, V>::value>::type
			write(Serialiser& s, std::ostream& out, const std::map<K, V>& m)
		{
			write(s, out, (uint32_t)m.size());
			for (const auto &it : m)
			{
				write(s, out, it.first);
				write(s, out, it.second);
			}
		}

		template<typename K, typename V>
		inline typename std::enable_if<!std::is_base_of<IReflect, V>::value>::type
			read(Unserialiser& u, std::istream& in, std::map<K, V>& m)
		{
			uint32_t count;
			read(u, in, count);

			for (uint32_t i = 0; i < count; i++)
			{
				K k;
				V v;

				read(u, in, k);
				read(u, in, v);

				m.insert(std::pair<K, V>(std::move(k), std::move(v)));
			}
		}

		inline void skip_map(Unserialiser& u, std::istream& in, const std::string_view& map_type)
		{
			uint32_t count;
			read(u, in, count);

			const std::string_view map_string("std::map<");
			const std::string_view map_parms(map_type.data() + map_string.length(), map_type.length() - map_string.length() - 1);	// -1 for tailing '>'.

			const auto comma = map_parms.find(',');
			const std::string_view key_type = map_parms.substr(0, comma);
			const std::string_view value_type = map_parms.substr(comma + 1);

			for (uint32_t i = 0; i < count; i++)
			{
				SkipField(u, in, key_type);
				SkipField(u, in, value_type);
			}
		}

		//----------------------------------------------------------------------
		// IReflect templates.
		// These need to be specialised because they call Unserialise() instead of >>.

		inline void skip_ref(Unserialiser& u, std::istream& in, const std::string_view& ref_type)
		{
			uint32_t count;
			read(u, in, count);

			std::string value_type;
			read(u, in, value_type);

			SkipField(u, in, value_type);
		}

		//
		// IReflect
		//
		inline void write(Serialiser& s, std::ostream& out, const IReflect& v)
		{
			s.AddSchema(*v.GetClass());
			v.Serialise(s, out);
		}

		inline void read(Unserialiser& u, std::istream& in, IReflect& v)
		{
			v.Unserialise(u, in);
		}

		//
		// Vector (IReflect).
		//
		template<typename T>
		inline typename std::enable_if<std::is_base_of<IReflect, T>::value>::type
			write(Serialiser& s, std::ostream& out, const std::vector<T>& v)
		{
			s.AddSchema(T::StaticClass());
			write(s, out, (uint32_t)v.size());
			for (const auto &it : v)
			{
				it->Serialise(s, out);
			}
		}

		template<typename T>
		inline typename std::enable_if<std::is_base_of<IReflect, T>::value>::type
			read(Unserialiser& u, std::istream& in, std::vector<T>& v)
		{
			uint32_t count;
			read(u, in, count);

			for (uint32_t i = 0; i < count; i++)
			{
				T t(&T::StaticClass());
				t.Unserialise(u, in);
				v.push_back(std::move(t));
			}
		}

		//
		// Map (IReflect).
		//
		template<typename K, typename V>
		inline typename std::enable_if<std::is_base_of<IReflect, K>::value>::type
			write(Serialiser& s, std::ostream& out, const std::map<K, V>& m)
		{
			s.AddSchema(V::StaticClass());
			write(s, out, (uint32_t)m.size());
			for (const auto& it : m)
			{
				write(s, out, it.first);
				it.second.Serialise(s, out);
			}
		}

		template<typename K, typename V>
		inline typename std::enable_if<std::is_base_of<IReflect, K>::value>::type
			read(Unserialiser& u, std::istream& in, std::map<K, V>& m)
		{
			uint32_t count;
			read(u, in, count);

			for (uint32_t i = 0; i < count; i++)
			{
				K k;
				read(u, in, k);

				V v(&V::StaticClass());
				v.Unserialise(u, in);
				m.insert(std::pair<K, V>(std::move(k), std::move(v)));
			}
		}

		//----------------------------------------------------------------------
		// Misc templates (implementation!).

		inline void skip_user_data_type(Unserialiser& u, std::istream& in, const UserDataType* udt)
		{
			in.ignore(udt->sz);
		}

		//
		// Strings are encoded as string pool indices.
		//
		inline void write(Serialiser& s, std::ostream& out, const std::string& v)
		{
			write(s, out, std::string_view(v));
		}

		inline void read(Unserialiser& u, std::istream& in, std::string& v)
		{
			StringPool::index_t index;
			read(u, in, index);
			v = u.GetStringPool().At(index);
		}

		inline void write(Serialiser& s, std::ostream& out, const std::string_view& v)
		{
			write(s, out, s.AddString(v));
		}

		//
		// String pool.
		//
		inline void write(Serialiser& s, std::ostream& out, const StringPool& p)
		{
			p.Serialise(s, out);
		}

		inline void read(Unserialiser& u, std::istream& in, StringPool& p)
		{
			p.Unserialise(u, in);
		}

		//
		// FieldSchema.
		//
		inline void write(Serialiser& s, std::ostream& out, const FieldSchema& f)
		{
			write(s, out, f.name);
			write(s, out, f.type);
			write(s, out, f.fields);
		}

		inline void read(Unserialiser& u, std::istream& in, FieldSchema& f)
		{
			read(u, in, f.name);
			read(u, in, f.type);
			read(u, in, f.fields);
		}

		//
		// UserDataType.
		//
		inline void write(Serialiser& s, std::ostream& out, const UserDataType& t)
		{
			write(s, out, t.name);
			write(s, out, t.sz);
		}

		inline void read(Unserialiser& u, std::istream& in, UserDataType& t)
		{
			read(u, in, t.name);
			read(u, in, t.sz);
		}
	}

	template<typename T>
	void WriteField(Serialiser& s, std::ostream& out, const T& v)
	{
		FieldImpl::write(s, out, v);
	}

	template<typename T, size_t offset>
	void ReadField(Unserialiser& u, std::istream& in, void* self)
	{
		FieldImpl::read(u, in, *(T *)((char*)self + offset));
	}

	template<typename TSerialiser, typename T>
	void WriteCustomField(Serialiser& s, std::ostream& out, const T& v)
	{
		s.AddUserDataType<T>(TSerialiser::value_size);
		TSerialiser::Serialise(s, out, v);
	}

	template<typename TSerialiser, typename T, size_t offset>
	void ReadCustomField(Unserialiser& u, std::istream& in, void* self)
	{
		TSerialiser::Unserialise(u, in, *(T*)((char*)self + offset));
	}

	inline void SkipField(Unserialiser& u, std::istream& in, const std::string_view& type)
	{
		//
		// Skip atomic types - very easy.
		//
		if (type == "char" || type == "unsigned char")					FieldImpl::skip<char>(u, in);
		else if (type == "short" || type == "unsigned short")			FieldImpl::skip<short>(u, in);
		else if (type == "int" || type == "unsigned int")				FieldImpl::skip<int>(u, in);
		else if (type == "long" || type == "unsigned long")				FieldImpl::skip<long>(u, in);
		else if (type == "long long" || type == "unsigned long long")	FieldImpl::skip<long long>(u, in);
		else if (type == "float")										FieldImpl::skip<float>(u, in);
		else if (type == "double")										FieldImpl::skip<double>(u, in);
		else if (type == "std::string")									FieldImpl::skip<StringPool::index_t>(u, in);
		else if (type.find("std::vector<") == 0)						FieldImpl::skip_vector(u, in, type);
		else if (type.find("std::map<") == 0)							FieldImpl::skip_map(u, in, type);
		else if (type.find("Ref<") == 0)								FieldImpl::skip_ref(u, in, type);
		else
		{
			// Try the userdata schema (probably smaller list to search...).
			const auto* userDataType = u.GetUserDataType(std::string(type));
			if (userDataType)
			{
				FieldImpl::skip_user_data_type(u, in, userDataType);
				return;
			}

			// This will throw if the schema can't be found. Honestly, it's for the best.
			// We don't know what the type is. This message can't be dealt with...
			const auto &schema = u.GetSchema(std::string(type));
			for (const auto& f : schema.fields)
			{
				SkipField(u, in, f.type);
			}
		}
	}

	//--------------------------------------------------------------------------
	// StringPool implementation.
	//--------------------------------------------------------------------------

	inline StringPool::entry_t::entry_t(Unserialiser& u, std::istream& fin)
	{
		FieldImpl::read(u, fin, offset);
		FieldImpl::read(u, fin, length);
	}

	inline void StringPool::entry_t::operator()(Serialiser& s, std::ostream& fout) const
	{
		FieldImpl::write(s, fout, offset);
		FieldImpl::write(s, fout, length);
	}

	inline StringPool::index_t StringPool::Add(const std::string_view& view)
	{
		const std::string s(view);
		const auto it = m_cache.find(std::string(s));
		if (it != m_cache.end())
		{
			return it->second;
		}

		m_cache[s] = (index_t)m_entries.size();
		m_entries.push_back(entry_t((uint32_t)m_pool.length(), (uint32_t)view.length()));
		m_pool.append(view);

		return (index_t)m_entries.size() - 1;
	}

	inline const std::string_view StringPool::At(index_t i) const
	{
		if (i < 0 || i >= m_entries.size())
		{
			throw std::out_of_range("String pool index out of range");
		}

		const auto& e = m_entries.at(i);
		return std::string_view(m_pool.c_str() + e.offset, e.length);
	}

	inline void StringPool::Serialise(Serialiser& s, std::ostream& fout) const
	{
		FieldImpl::write(s, fout, (uint32_t)m_pool.length());
		fout.write(m_pool.c_str(), m_pool.length());

		FieldImpl::write(s, fout, (uint32_t)m_entries.size());
		for (index_t i = 0; i < m_entries.size(); i++)
		{
			m_entries[i](s, fout);
		}
	}

	inline void StringPool::Unserialise(Unserialiser& u, std::istream& fin)
	{
		index_t count, length;
		char buf[256];

		// Reserve memory for the string pool.
		// Means malloc cost is minimized - just need to optimise for input.
		// Don't use resize()! That will initialise each character!
		FieldImpl::read(u, fin, length);
		m_pool.reserve(length);

		// Read the string in 256 char chunks.
		while (length)
		{
			index_t read_sz = std::min(length, (index_t)sizeof(buf));
			fin.read(buf, read_sz);
			m_pool.append(buf, length);
			length -= read_sz;
		}

		// Read the pool metadata.
		FieldImpl::read(u, fin, count);
		m_entries.reserve(count);
		for (index_t i = 0; i < count; i++)
		{
			m_entries.push_back(entry_t(u, fin));
		}
	}

	//--------------------------------------------------------------------------
	// FieldSchema implementation.
	//--------------------------------------------------------------------------

	inline FieldSchema::FieldSchema(const Reflect::Class* static_class, StringPool& pool)
		: type(static_class->GetName())
	{
		// Yes, technically 'name' is empty here. But we need to ensure the string
		// pool has an empty string for deserialisation!
		pool.Add(name);
		pool.Add(type);

		for (const auto& f : static_class->GetMembers({ "serialise" }, false))
		{
			fields.push_back(FieldSchema(f, pool));
		}
	}

	inline FieldSchema::FieldSchema(const Reflect::ReflectMember& member, StringPool& pool)
		: name(member.GetName())
		, type(member.GetTypeName())
	{
		pool.Add(name);
		pool.Add(type);
	}
}