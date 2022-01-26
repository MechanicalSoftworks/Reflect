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

		StringPool::index_t AddString(const std::string_view& view) { return m_string_pool.Add(view); }

	private:
		std::map<std::string, FieldSchema> m_schemas;
		StringPool m_string_pool;
	};

	//
	// Parses a message and recreates the objects.
	//
	typedef void* (*AlignedAlloc)(size_t size, size_t alignment);
	typedef void  (*AlignedFree)(void *ptr);
	class REFLECT_DLL Unserialiser
	{
	public:
		Unserialiser(const Unserialiser&) = delete;
		Unserialiser(const Unserialiser&&) = delete;
		Unserialiser(AlignedAlloc alloc, AlignedFree free);
		~Unserialiser();

		void Read(std::istream& in);

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
	// Field serialisation.
	//--------------------------------------------------------------------------

	namespace FieldImpl
	{
		//----------------------------------------------------------------------
		// Misc templates (declarations).
		// These are used by the generic templates. Need to be known to the compiler.

		inline void write(Serialiser& s, std::ostream& out, const std::string& v);
		inline void read(Unserialiser& u, std::istream& in, std::string& v);
		inline void write(Serialiser& s, std::ostream& out, const StringPool& p);
		inline void read(Unserialiser& u, std::istream& in, StringPool& p);
		inline void write(Serialiser& s, std::ostream& out, const FieldSchema& f);
		inline void read(Unserialiser& u, std::istream& in, FieldSchema& f);

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
			out.write((const char *)&v, sizeof(v));
		}

		template<typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
		inline typename std::enable_if<!std::is_base_of<IReflect, T>::value>::type
			read(Unserialiser& u, std::istream& in, T& v)
		{
			in.read((char *)&v, sizeof(v));
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

		//----------------------------------------------------------------------
		// IReflect templates.
		// These need to be specialised because they call Unserialise() instead of >>.

		//
		// IReflect
		//
		inline void read(Serialiser& s, std::ostream& out, const IReflect& v)
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
				write(s, out, it->first);
				it->second.Serialise(s, out);
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

		//
		// Strings are encoded as string pool indices.
		//
		inline void write(Serialiser& s, std::ostream& out, const std::string& v)
		{
			write(s, out, s.AddString(v));
		}

		inline void read(Unserialiser& u, std::istream& in, std::string& v)
		{
			StringPool::index_t index;
			read(u, in, index);
			v = u.GetStringPool().At(index);
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
	}

	template<typename T>
	void WriteField(Unserialiser& u, std::ostream& out, const T& v)
	{
		FieldImpl::write(u, out, v);
	}

	template<typename T, size_t offset>
	void ReadField(Unserialiser& u, std::istream& in, void* self)
	{
		FieldImpl::read(u, in, *(T *)((char*)self + offset));
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
			m_pool.append(buf);
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