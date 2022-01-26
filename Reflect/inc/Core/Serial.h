#pragma once

#include "Reflect.h"

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
	class IReflect;

	//
	// Handles string de-deplication.
	//
	class StringPool
	{
		struct entry_t
		{
			entry_t(uint32_t off, uint32_t len) : offset(off), length(len) {}

			entry_t(std::istream& fin)                { fin  >> offset >> length; }
			void operator()(std::ostream& fout) const { fout << offset << length; }

			uint32_t		offset;
			uint32_t		length;
		};

	public:
		// 4 billion strings should be enough, right?
		typedef uint32_t index_t;

		index_t add(const std::string_view& view);
		const std::string_view at(index_t i) const;

		void Serialise(std::ostream& fout) const;
		void Unserialise(std::istream& fin);

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

		FieldSchema(Reflect::Class* static_class, StringPool& pool);
		FieldSchema(const Reflect::ReflectMember& member, StringPool& pool);

		std::string name, type;
		std::vector<FieldSchema> fields;
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
	class Serialiser
	{
	public:
		Serialiser(std::ostream &fout, const IReflect &root);

		// Include schemas for each object.
		// Allows us to load older messages (such as saved project files).
		void AddSchema(Reflect::Class* static_class);

	private:
		std::map<std::string, FieldSchema> m_schemas;
		StringPool m_string_pool;
	};

	//
	// Parses a message and recreates the objects.
	//
	typedef void* (*AlignedAlloc)(size_t size, size_t alignment);
	typedef void* (*AlignedFree)(void *ptr);
	class Unserialiser
	{
	public:
		Unserialiser(std::istream& in, AlignedAlloc alloc, AlignedFree free);
		~Unserialiser();

		IReflect* Detach();
		const StringPool& GetStringPool() const { return m_string_pool; }

	private:
		std::map<std::string, FieldSchema> m_schemas;
		StringPool m_string_pool;

		AlignedAlloc m_alloc;
		AlignedFree m_free;

		IReflect* m_root = nullptr;
		Reflect::Class* m_root_class = nullptr;
	};

	// Helpers for reading individual fields.
	typedef void (*ReadFieldType)(Unserialiser &u, std::istream& s, void* self);
	struct UnserialiseField
	{
		constexpr UnserialiseField(const char* n, ReadFieldType r) : name(n), read(r) {}
		const char* name;
		ReadFieldType read;
	};

	//--------------------------------------------------------------------------
	// Field unserialisation.

	namespace ReadFieldImpl
	{
		//----------------------------------------------------------------------
		// Generic templates.
		
		// Reads a field from the stream into the given member.
		// Disabled for IReflect types because they need to go through their own overload, below.
		template<typename T>
		inline typename std::enable_if<!std::is_base_of<IReflect, T>::value>::type 
			impl(Unserialiser& u, std::istream& in, T& v)
		{
			in >> v;
		}

		template<typename T>
		inline typename std::enable_if<!std::is_base_of<IReflect, T>::value>::type
			impl(Unserialiser& u, std::istream& in, std::vector<T>& v)
		{
			uint32_t count;
			in >> count;

			v.reserve(count);

			for (uint32_t i = 0; i < count; i++)
			{
				T t;
				impl(u, in, t);
				v.push_back(std::move(t));
			}
		}

		template<typename K, typename V>
		inline typename std::enable_if<!std::is_base_of<IReflect, V>::value>::type
			impl(Unserialiser& u, std::istream& in, std::map<K, V>& m)
		{
			uint32_t count;
			in >> count;

			for (uint32_t i = 0; i < count; i++)
			{
				K k;
				impl(u, in, k);

				V v;
				impl(u, in, v);
				m.insert(std::pair<K, V>(std::move(k), std::move(v)));
			}
		}

		//----------------------------------------------------------------------
		// IReflect templates.
		// These need to be specialised because they call Unserialise() instead of >>.

		inline void impl(Unserialiser& u, std::istream& in, IReflect& v)
		{
			v.Unserialise(u, in);
		}

		template<typename T>
		inline typename std::enable_if<std::is_base_of<IReflect, T>::value>::type
			impl(Unserialiser& u, std::istream& in, std::vector<T>& v)
		{
			uint32_t count;
			in >> count;

			for (uint32_t i = 0; i < count; i++)
			{
				T t(&T::StaticClass());
				t.Unserialise(u, in);
				v.push_back(std::move(t));
			}
		}

		// Remember, the default template is disabled for IReflect types.
		// Forces them to go down this path.
		template<typename K, typename V>
		inline typename std::enable_if<std::is_base_of<IReflect, K>::value>::type
			impl(Unserialiser& u, std::istream& in, std::map<K, V>& m)
		{
			uint32_t count;
			in >> count;

			for (uint32_t i = 0; i < count; i++)
			{
				K k;
				impl(u, in, k);

				V v(&T::StaticClass());
				v.Unserialise(u, in);
				m.insert(std::pair<K, V>(std::move(k), std::move(v)));
			}
		}

		//----------------------------------------------------------------------
		// Misc templates.

		// Strings are encoded as string pool indices.
		inline void impl(Unserialiser& u, std::istream& in, std::string& v)
		{
			StringPool::index_t index;
			in >> index;
			v = u.GetStringPool().at(index);
		}

		inline void impl(Unserialiser& u, std::istream& in, StringPool& s)
		{
			s.Unserialise(in);
		}

		inline void impl(Unserialiser& u, std::istream& in, FieldSchema& s)
		{
			impl(u, in, s.name);
			impl(u, in, s.type);
			impl(u, in, s.fields);
		}
	}

	template<typename T, size_t offset>
	void ReadField(Unserialiser& u, std::istream& in, void* self)
	{
		ReadFieldImpl::impl(u, in, *(T *)((char*)self + offset));
	}

	//--------------------------------------------------------------------------
	// StringPool implementation.
	//--------------------------------------------------------------------------

	inline StringPool::index_t StringPool::add(const std::string_view& view)
	{
		const std::string s(view);
		const auto it = m_cache.find(std::string(s));
		if (it != m_cache.end())
		{
			return it->second;
		}

		m_pool.append(view);
		m_cache[s] = (index_t)m_entries.size();
		m_entries.push_back(entry_t((uint32_t)m_pool.length(), (uint32_t)view.length()));

		return (index_t)m_entries.size() - 1;
	}

	inline const std::string_view StringPool::at(index_t i) const
	{
		if (i < 0 || i >= m_entries.size())
		{
			throw std::out_of_range("String pool index out of range");
		}

		const auto& e = m_entries.at(i);
		return std::string_view(m_pool.c_str() + e.offset, e.length);
	}

	inline void StringPool::Serialise(std::ostream& fout) const
	{
		fout << (uint32_t)m_pool.length();
		fout.write(m_pool.c_str(), m_pool.length());

		fout << (uint32_t)m_entries.size();
		for (index_t i = 0; i < m_entries.size(); i++)
		{
			m_entries[i](fout);
		}
	}

	inline void StringPool::Unserialise(std::istream& fin)
	{
		index_t count, length;
		char buf[256];

		// Reserve memory for the string pool.
		// Means malloc cost is minimized - just need to optimise for input.
		// Don't use resize()! That will initialise each character!
		fin >> length;
		m_pool.reserve(length);

		// Read the string in 256 char chunks.
		while (length)
		{
			index_t read_sz = std::min(length, (index_t)sizeof(buf));
			fin.getline(buf, read_sz);
			m_pool.append(buf);
			length -= read_sz;
		}

		// Read the pool metadata.
		fin >> count;
		m_entries.reserve(count);
		for (index_t i = 0; i < count; i++)
		{
			m_entries.push_back(entry_t(fin));
		}
	}

	//--------------------------------------------------------------------------
	// FieldSchema implementation.
	//--------------------------------------------------------------------------

	inline FieldSchema::FieldSchema(Reflect::Class* static_class, StringPool& pool)
		: type(static_class->GetName())
	{
		pool.add(type);

		for (const auto& f : static_class->GetMembers({ "serialise" }, false))
		{
			fields.push_back(FieldSchema(f, pool));
		}
	}

	inline FieldSchema::FieldSchema(const Reflect::ReflectMember& member, StringPool& pool)
		: name(member.GetName())
		, type(member.GetTypeName())
	{
		pool.add(name);
		pool.add(type);
	}
}