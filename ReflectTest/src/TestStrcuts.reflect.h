 // This file is auto generated please don't modify.
#include "ReflectStructs.h"
#include "Core/Core.h"
#include "Core/Serial.h"
#include "Core/Util.h"
#include <array>

#ifdef TestStrcuts_reflect_h
#error "TestStrcuts_reflect.h already included, missing 'pragma once' in TestStrcuts.h"
#endif TestStrcuts_reflect_h
#define TestStrcuts_reflect_h

#define TestStrcuts_Source_h_26_STATIC_CLASS \
public:\
	typedef Reflect::IReflect SuperClass;\
	static const Reflect::Class StaticClass;\
	static void __PlacementNew(S* obj) { new(obj) S; }\
	static void __PlacementDelete(S* obj) { obj->~S(); }\


#define TestStrcuts_Source_h_26_PROPERTIES \
private:\
	static Reflect::ReflectMemberProp __REFLECT_MEMBER_PROPS__[2];\


#define TestStrcuts_Source_h_26_FUNCTION_DECLARE \
private:\


#define TestStrcuts_Source_h_26_FUNCTION_GET \
public:\
	virtual Reflect::ReflectFunction GetFunction(const std::string_view &functionName) override;\


#define TestStrcuts_Source_h_26_PROPERTIES_OFFSET \
private:\
	static constexpr int __REFLECT__Friends() { return offsetof(S, Friends); }; \
	static constexpr int __REFLECT__TimeOnline() { return offsetof(S, TimeOnline); }; \


#define TestStrcuts_Source_h_26_PROPERTIES_GET \
public:\
virtual Reflect::ReflectMember GetMember(const std::string_view& memberName) override;\
virtual std::vector<Reflect::ReflectMember> GetMembers(std::vector<std::string> const& flags) override;\


#define TestStrcuts_Source_h_26_DATA_DICTIONARY \
public:\


#define TestStrcuts_Source_h_26_SERIALISE_METHODS \
public:\
virtual void Serialise(Reflect::Serialiser &s, std::ostream &out) const override;\
virtual void Unserialise(Reflect::Unserialiser &u, std::istream &in) override;\


#define TestStrcuts_Source_h_26_GENERATED_BODY \
TestStrcuts_Source_h_26_STATIC_CLASS \
TestStrcuts_Source_h_26_PROPERTIES \
TestStrcuts_Source_h_26_FUNCTION_DECLARE \
TestStrcuts_Source_h_26_FUNCTION_GET \
TestStrcuts_Source_h_26_PROPERTIES_OFFSET \
TestStrcuts_Source_h_26_PROPERTIES_GET \
TestStrcuts_Source_h_26_DATA_DICTIONARY \
TestStrcuts_Source_h_26_SERIALISE_METHODS \


#define TestStrcuts_Source_h_42_STATIC_CLASS \
public:\
	typedef Reflect::IReflect SuperClass;\
	static const Reflect::Class StaticClass;\
	static void __PlacementNew(Actor* obj) { new(obj) Actor; }\
	static void __PlacementDelete(Actor* obj) { obj->~Actor(); }\


#define TestStrcuts_Source_h_42_PROPERTIES \
private:\
	static Reflect::ReflectMemberProp __REFLECT_MEMBER_PROPS__[0];\


#define TestStrcuts_Source_h_42_FUNCTION_DECLARE \
private:\


#define TestStrcuts_Source_h_42_FUNCTION_GET \
public:\
	virtual Reflect::ReflectFunction GetFunction(const std::string_view &functionName) override;\


#define TestStrcuts_Source_h_42_PROPERTIES_OFFSET \
private:\


#define TestStrcuts_Source_h_42_PROPERTIES_GET \
public:\
virtual Reflect::ReflectMember GetMember(const std::string_view& memberName) override;\
virtual std::vector<Reflect::ReflectMember> GetMembers(std::vector<std::string> const& flags) override;\


#define TestStrcuts_Source_h_42_DATA_DICTIONARY \
public:\


#define TestStrcuts_Source_h_42_SERIALISE_METHODS \
public:\
virtual void Serialise(Reflect::Serialiser &s, std::ostream &out) const override;\
virtual void Unserialise(Reflect::Unserialiser &u, std::istream &in) override;\


#define TestStrcuts_Source_h_42_GENERATED_BODY \
TestStrcuts_Source_h_42_STATIC_CLASS \
TestStrcuts_Source_h_42_PROPERTIES \
TestStrcuts_Source_h_42_FUNCTION_DECLARE \
TestStrcuts_Source_h_42_FUNCTION_GET \
TestStrcuts_Source_h_42_PROPERTIES_OFFSET \
TestStrcuts_Source_h_42_PROPERTIES_GET \
TestStrcuts_Source_h_42_DATA_DICTIONARY \
TestStrcuts_Source_h_42_SERIALISE_METHODS \


#define TestStrcuts_Source_h_54_STATIC_CLASS \
public:\
	typedef Actor SuperClass;\
	static const Reflect::Class StaticClass;\
	static void __PlacementNew(Player* obj) { new(obj) Player; }\
	static void __PlacementDelete(Player* obj) { obj->~Player(); }\


#define TestStrcuts_Source_h_54_PROPERTIES \
private:\
	static Reflect::ReflectMemberProp __REFLECT_MEMBER_PROPS__[2];\


#define TestStrcuts_Source_h_54_FUNCTION_DECLARE \
private:\
	static Reflect::ReflectReturnCode __REFLECT_FUNC__GetOnlineFriendsCount(void* objectPtr, void* returnValuePtr, Reflect::FunctionPtrArgs& functionArgs)\
	{\
		int const& maxPlayerCountArg = *static_cast<int const*>(functionArgs.GetArg(0));\
		Player* ptr = static_cast<Player*>(objectPtr);\
		*((int*)returnValuePtr) = ptr->GetOnlineFriendsCount(maxPlayerCountArg);\
		return Reflect::ReflectReturnCode::SUCCESS;\
	}\
	static Reflect::ReflectReturnCode __REFLECT_FUNC__PrintHelloWorld(void* objectPtr, void* returnValuePtr, Reflect::FunctionPtrArgs& functionArgs)\
	{\
		Player* ptr = static_cast<Player*>(objectPtr);\
		ptr->PrintHelloWorld();\
		return Reflect::ReflectReturnCode::SUCCESS;\
	}\
	static Reflect::ReflectReturnCode __REFLECT_FUNC__GetId(void* objectPtr, void* returnValuePtr, Reflect::FunctionPtrArgs& functionArgs)\
	{\
		Player* ptr = static_cast<Player*>(objectPtr);\
		*((std::string*)returnValuePtr) = ptr->GetId();\
		return Reflect::ReflectReturnCode::SUCCESS;\
	}\


#define TestStrcuts_Source_h_54_FUNCTION_GET \
public:\
	virtual Reflect::ReflectFunction GetFunction(const std::string_view &functionName) override;\


#define TestStrcuts_Source_h_54_PROPERTIES_OFFSET \
private:\
	static constexpr int __REFLECT__Friends() { return offsetof(Player, Friends); }; \
	static constexpr int __REFLECT__TimeOnline() { return offsetof(Player, TimeOnline); }; \


#define TestStrcuts_Source_h_54_PROPERTIES_GET \
public:\
virtual Reflect::ReflectMember GetMember(const std::string_view& memberName) override;\
virtual std::vector<Reflect::ReflectMember> GetMembers(std::vector<std::string> const& flags) override;\


#define TestStrcuts_Source_h_54_DATA_DICTIONARY \
public:\


#define TestStrcuts_Source_h_54_SERIALISE_METHODS \
public:\
virtual void Serialise(Reflect::Serialiser &s, std::ostream &out) const override;\
virtual void Unserialise(Reflect::Unserialiser &u, std::istream &in) override;\


#define TestStrcuts_Source_h_54_GENERATED_BODY \
TestStrcuts_Source_h_54_STATIC_CLASS \
TestStrcuts_Source_h_54_PROPERTIES \
TestStrcuts_Source_h_54_FUNCTION_DECLARE \
TestStrcuts_Source_h_54_FUNCTION_GET \
TestStrcuts_Source_h_54_PROPERTIES_OFFSET \
TestStrcuts_Source_h_54_PROPERTIES_GET \
TestStrcuts_Source_h_54_DATA_DICTIONARY \
TestStrcuts_Source_h_54_SERIALISE_METHODS \


#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID TestStrcuts_Source_h


enum class E : int;
inline const char* EnumToString(E x) {
	switch((int)x) {
		case 0: return "Value1";
		case 1: return "Value2";
		case 10: return "Value3";
		case 11: return "Value4";
	}
	return nullptr;
}

inline bool StringToEnum(const std::string_view &str, E& x) {
	if (str == "Value1") { x = E(0); return true; }
	if (str == "Value2") { x = E(1); return true; }
	if (str == "Value3") { x = E(10); return true; }
	if (str == "Value4") { x = E(11); return true; }
	return false;
}
