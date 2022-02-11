#pragma once

#include "Reflect.h"
#include "Core/Allocator.h"

#include <map>
#include <stack>

namespace Reflect
{
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
		inline typename std::enable_if_t<!std::is_base_of_v<IReflect, T>>
			write(Serialiser& s, std::ostream& out, const T& v)
		{
			out.write((const char*)&v, sizeof(v));
		}

		template<typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
		inline typename std::enable_if_t<!std::is_base_of_v<IReflect, T>>
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
		// Enum types.
		//
		template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
		inline void write(Serialiser& s, std::ostream& out, const T& v)
		{
			s.AddUserDataType<T>(sizeof(T));
			out.write((const char*)&v, sizeof(v));
		}

		template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
		inline void read(Unserialiser& u, std::istream& in, T& v)
		{
			in.read((char*)&v, sizeof(v));
		}

		//
		// Vector types (non IReflect).
		//
		template<typename T>
		inline typename std::enable_if_t<!std::is_base_of_v<IReflect, T>>
			write(Serialiser& s, std::ostream& out, const std::vector<T>& v)
		{
			write(s, out, (uint32_t)v.size());
			for (const auto &it : v)
			{
				write(s, out, it);
			}
		}

		template<typename T>
		inline typename std::enable_if_t<!std::is_base_of_v<IReflect, T>>
			read(Unserialiser& u, std::istream& in, std::vector<T>& v)
		{
			v.clear();

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
		inline typename std::enable_if_t<!std::is_base_of_v<IReflect, V>>
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
		inline typename std::enable_if_t<!std::is_base_of_v<IReflect, V>>
			read(Unserialiser& u, std::istream& in, std::map<K, V>& m)
		{
			m.clear();

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

		inline void write(Serialiser& s, std::ostream& out, const IReflect* v) { write(s, out, *v); }
		inline void read(Unserialiser& u, std::istream& in, IReflect* v) { read(u, in, *v); }

		//
		// Vector (IReflect).
		//
		template<typename T>
		inline typename std::enable_if_t<std::is_base_of_v<IReflect, T>>
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
		inline typename std::enable_if_t<std::is_base_of_v<IReflect, T>>
			read(Unserialiser& u, std::istream& in, std::vector<T>& v)
		{
			v.clear();

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
		inline typename std::enable_if_t<std::is_base_of_v<IReflect, K>>
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
		inline typename std::enable_if_t<std::is_base_of_v<IReflect, K>>
			read(Unserialiser& u, std::istream& in, std::map<K, V>& m)
		{
			m.clear();

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
		else if (type == "__int64" || type == "unsigned __int64" || type == "long long" || type == "unsigned long long")	FieldImpl::skip<long long>(u, in);
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
}