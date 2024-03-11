 // This file is auto generated please don't modify.
#include "ReflectStructs.h"
#include "Core/Core.h"
#include "Core/Enums.h"
#include "Core/Util.h"
#include "ReflectStatic.h"

#ifdef C__mechsrc_terapixel_Dev_ThirdParty_Reflect_ReflectTest_src_TestStrcuts_h_reflect_static_h
#error "C__mechsrc_terapixel_Dev_ThirdParty_Reflect_ReflectTest_src_TestStrcuts_h_reflect_static.h already included, missing 'pragma once' in TestStrcuts.h"
#endif //C__mechsrc_terapixel_Dev_ThirdParty_Reflect_ReflectTest_src_TestStrcuts_h_reflect_static_h
#define C__mechsrc_terapixel_Dev_ThirdParty_Reflect_ReflectTest_src_TestStrcuts_h_reflect_static_h

namespace Reflect {

	template<> struct ReflectStatic<S> {
		static inline constexpr auto Properties = std::make_tuple(
			make_static_field<int>("Friends", S::__OFFSETOF__Friends(), std::make_tuple("EditorOnly", "Public")), 
			make_static_field<int>("TimeOnline", S::__OFFSETOF__TimeOnline(), std::make_tuple("Public"))
		);
	};

	template<typename T> struct ReflectStatic<TemplatedClass<T>> {
		static inline constexpr auto Properties = std::make_tuple(
			make_static_field<T>("Property", TemplatedClass<T>::__OFFSETOF__Property(), std::make_tuple("EditorOnly", "Public"))
		);
	};

	template<> struct ReflectStatic<Actor> {
		static inline constexpr auto Properties = std::make_tuple(
		);
	};

	template<> struct ReflectStatic<Player> {
		static inline constexpr auto Properties = std::make_tuple(
			make_static_field<std::string>("Id", Player::__OFFSETOF__Id(), std::make_tuple("EditorOnly", "Public")), 
			make_static_field<int>("Friends", Player::__OFFSETOF__Friends(), std::make_tuple("EditorOnly", "Public")), 
			make_static_field<int>("TimeOnline", Player::__OFFSETOF__TimeOnline(), std::make_tuple("Public"))
		);
	};

} // namespace Reflect

