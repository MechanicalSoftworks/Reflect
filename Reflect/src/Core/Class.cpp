#include "ReflectStructs.h"

#include <map>

namespace Reflect
{
	static std::map<std::string, Class*> s_classes;

	REFLECT_DLL void Class::Register(Class* c)
	{
		s_classes[c->m_name] = c;
	}

	REFLECT_DLL Class* Class::Lookup(const char* name)
	{
		auto it = s_classes.find(name);
		return it != s_classes.end() ? it->second : nullptr;
	}
}
