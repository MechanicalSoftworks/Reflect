 // This file is auto generated please don't modify.
#include "ReflectStructs.h"
#include "Core/Core.h"
#include "Core/Enums.h"
#include "Core/Util.h"
#include "ReflectStatic.h"
#include <array>

#ifdef C__mechsrc_terapixel_1_Dev_ThirdParty_Reflect_ReflectTest_src_TestStrcuts_h_reflect_static_h
#error "C__mechsrc_terapixel_1_Dev_ThirdParty_Reflect_ReflectTest_src_TestStrcuts_h_reflect_static.h already included, missing 'pragma once' in TestStrcuts.h"
#endif //C__mechsrc_terapixel_1_Dev_ThirdParty_Reflect_ReflectTest_src_TestStrcuts_h_reflect_static_h
#define C__mechsrc_terapixel_1_Dev_ThirdParty_Reflect_ReflectTest_src_TestStrcuts_h_reflect_static_h

template<> struct Reflect::ReflectStatic<S> {
	static inline constexpr auto Properties = std::make_tuple(
		Reflect::make_static_field<int>("Friends", S::__OFFSETOF__Friends(), std::make_tuple("EditorOnly", "Public")), 
		Reflect::make_static_field<int>("TimeOnline", S::__OFFSETOF__TimeOnline(), std::make_tuple("Public"))
	);
};

template<> struct Reflect::ReflectStatic<Actor> {
	static inline constexpr auto Properties = std::make_tuple(
	);
};

template<> struct Reflect::ReflectStatic<Player> {
	static inline constexpr auto Properties = std::make_tuple(
		Reflect::make_static_field<std::string>("Id", Player::__OFFSETOF__Id(), std::make_tuple("EditorOnly", "Public")), 
		Reflect::make_static_field<int>("Friends", Player::__OFFSETOF__Friends(), std::make_tuple("EditorOnly", "Public")), 
		Reflect::make_static_field<int>("TimeOnline", Player::__OFFSETOF__TimeOnline(), std::make_tuple("Public"))
	);
};

