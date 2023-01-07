#include "ReflectStructs.h"

#include <map>
#include <iostream>

namespace Reflect
{
	using class_map_t = std::map<std::string, Class*>;

	// Static initialisation order is important, but can't be guaranteed.
	// So we allocate stack memory for the map, and manually invoke the class constructor when its used.
	// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Nifty_Counter
	static typename std::aligned_storage<sizeof(class_map_t), alignof (class_map_t)>::type s_class_map_buffer;

	static int s_class_map_counter = 0;
	static class_map_t& s_classes = reinterpret_cast<class_map_t&> (s_class_map_buffer);

	REFLECT_DLL LinkClass::LinkClass(const Class& c)
		: m_class(c)
	{
		if (s_class_map_counter++ == 0)
		{
			new (&s_classes) class_map_t();
		}

		if (!s_classes.try_emplace(c.Name, const_cast<Class*>(&c)).second)
		{
			throw std::runtime_error(std::string("Class '") + c.Name + "' is already registered");
		}
	}

	REFLECT_DLL LinkClass::~LinkClass()
	{
		s_classes.erase(m_class.Name);

		if (--s_class_map_counter == 0)
		{
			s_classes.~map();
		}
	}

	REFLECT_DLL void Class::RegisterOverride(const char* name, const Class& c)
	{
		s_classes[name] = (Class *)&c;
	}

	REFLECT_DLL const Class* Class::Lookup(const std::string_view& name)
	{
		auto it = s_classes.find(std::string(name));
		return it != s_classes.end() ? it->second : nullptr;
	}

	REFLECT_DLL std::vector<std::reference_wrapper<Class>> Class::LookupWhere(const std::function<bool(const Class&)>& pred)
	{
		std::vector<std::reference_wrapper<Class>> classes;
		for (const auto& c : s_classes)
		{
			if (pred(*c.second))
			{
				classes.push_back(*c.second);
			}
		}
		return classes;
	}

	REFLECT_DLL std::vector<std::reference_wrapper<Class>> Class::LookupDescendantsOf(const Class& super)
	{
		std::vector<std::reference_wrapper<Class>> descendants;

		for (const auto& it : s_classes)
		{
			// Not a descendant of yourself, silly!
			if (it.second == &super)
			{
				continue;
			}

			for (const Class* c = it.second; c != nullptr; c = c->SuperClass)
			{
				if (c == &super)
				{
					descendants.push_back(*it.second);
					break;
				}
			}
		}

		return descendants;
	}
}
