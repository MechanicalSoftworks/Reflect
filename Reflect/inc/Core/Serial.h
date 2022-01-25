#pragma once

#include "Reflect.h"

#include <map>

namespace Reflect
{
	//
	//
	//
	class Unserialise
	{
	public:
		// TODO: This also needs to take an object allocator.
		Unserialise(std::istream& in) {}
	};

	typedef void (*ReadFieldType)(Unserialise &u, std::istream& s, void* self);
	struct UnserialiseField
	{
		constexpr UnserialiseField(const char* n, ReadFieldType r) : name(n), read(r) {}
		const char* name;
		ReadFieldType read;
	};

	//--------------------------------------------------------------------------
	// Field unserialisation.

	namespace detail
	{
		// Reads a field from the stream into the given member.
		// Disabled for IReflect types because they need to go through their own overload, below.
		template<typename T>
		inline typename std::enable_if<!std::is_base_of<IReflect, T>::value>::type impl(Unserialise& u, std::istream& in, T& v)
		{
			in >> v;
		}

		inline void impl(Unserialise& u, std::istream& in, IReflect& v)
		{
			v.Unserialise(in);
		}

		template<typename T>
		inline void impl(Unserialise& u, std::istream& in, std::vector<T>& v)
		{
			// TODO
		}

		template<typename K, typename V>
		inline void impl(Unserialise& u, std::istream& in, std::map<K, V>& v)
		{
			// TODO
		}

		// Strings are encoded as string pool indices.
		inline void impl(Unserialise& u, std::istream& in, std::string& v)
		{
			// TODO: Read string index.
		}
	}

	template<typename T, size_t offset>
	void ReadField(Unserialise& u, std::istream& in, void* self)
	{
		detail::impl(u, in, *(T *)((char*)self + offset));
	}
}