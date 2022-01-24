 // This file is auto generated please don't modify.
#include "ReflectStructs.h"
#include "Core/Util.h"

#ifdef TestStrcuts_reflect_h
#error "TestStrcuts_reflect.h already included, missing 'pragma once' in TestStrcuts.h"
#endif TestStrcuts_reflect_h
#define TestStrcuts_reflect_h

#define TestStrcuts_Source_h_15_STATIC_CLASS \
public:\
	typedef Reflect::IReflect SuperClass;\
	static const Reflect::Class StaticClass;\
	static void __PlacementNew(S* obj) { new(obj) S; }\
	static void __PlacementDelete(S* obj) { obj->~S(); }\


#define TestStrcuts_Source_h_15_PROPERTIES \
private:\
	static Reflect::ReflectMemberProp __REFLECT_MEMBER_PROPS__[2];\


#define TestStrcuts_Source_h_15_FUNCTION_DECLARE \
private:\


#define TestStrcuts_Source_h_15_FUNCTION_GET \
public:\
	virtual Reflect::ReflectFunction GetFunction(const std::string_view &functionName) override;\


#define TestStrcuts_Source_h_15_PROPERTIES_OFFSET \
private:\
	static int __REFLECT__Friends() { return offsetof(S, Friends); }; \
	static int __REFLECT__TimeOnline() { return offsetof(S, TimeOnline); }; \


#define TestStrcuts_Source_h_15_PROPERTIES_GET \
public:\
virtual Reflect::ReflectMember GetMember(const std::string_view& memberName) override;\
virtual std::vector<Reflect::ReflectMember> GetMembers(std::vector<std::string> const& flags) override;\


#define TestStrcuts_Source_h_15_GENERATED_BODY \
TestStrcuts_Source_h_15_STATIC_CLASS \
TestStrcuts_Source_h_15_PROPERTIES \
TestStrcuts_Source_h_15_FUNCTION_DECLARE \
TestStrcuts_Source_h_15_FUNCTION_GET \
TestStrcuts_Source_h_15_PROPERTIES_OFFSET \
TestStrcuts_Source_h_15_PROPERTIES_GET \


#define TestStrcuts_Source_h_31_STATIC_CLASS \
public:\
	typedef Reflect::IReflect SuperClass;\
	static const Reflect::Class StaticClass;\
	static void __PlacementNew(Actor* obj) { new(obj) Actor; }\
	static void __PlacementDelete(Actor* obj) { obj->~Actor(); }\


#define TestStrcuts_Source_h_31_PROPERTIES \
private:\
	static Reflect::ReflectMemberProp __REFLECT_MEMBER_PROPS__[0];\


#define TestStrcuts_Source_h_31_FUNCTION_DECLARE \
private:\


#define TestStrcuts_Source_h_31_FUNCTION_GET \
public:\
	virtual Reflect::ReflectFunction GetFunction(const std::string_view &functionName) override;\


#define TestStrcuts_Source_h_31_PROPERTIES_OFFSET \
private:\


#define TestStrcuts_Source_h_31_PROPERTIES_GET \
public:\
virtual Reflect::ReflectMember GetMember(const std::string_view& memberName) override;\
virtual std::vector<Reflect::ReflectMember> GetMembers(std::vector<std::string> const& flags) override;\


#define TestStrcuts_Source_h_31_GENERATED_BODY \
TestStrcuts_Source_h_31_STATIC_CLASS \
TestStrcuts_Source_h_31_PROPERTIES \
TestStrcuts_Source_h_31_FUNCTION_DECLARE \
TestStrcuts_Source_h_31_FUNCTION_GET \
TestStrcuts_Source_h_31_PROPERTIES_OFFSET \
TestStrcuts_Source_h_31_PROPERTIES_GET \


#define TestStrcuts_Source_h_43_STATIC_CLASS \
public:\
	typedef Actor SuperClass;\
	static const Reflect::Class StaticClass;\
	static void __PlacementNew(Player* obj) { new(obj) Player; }\
	static void __PlacementDelete(Player* obj) { obj->~Player(); }\


#define TestStrcuts_Source_h_43_PROPERTIES \
private:\
	static Reflect::ReflectMemberProp __REFLECT_MEMBER_PROPS__[2];\


#define TestStrcuts_Source_h_43_FUNCTION_DECLARE \
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


#define TestStrcuts_Source_h_43_FUNCTION_GET \
public:\
	virtual Reflect::ReflectFunction GetFunction(const std::string_view &functionName) override;\


#define TestStrcuts_Source_h_43_PROPERTIES_OFFSET \
private:\
	static int __REFLECT__Friends() { return offsetof(Player, Friends); }; \
	static int __REFLECT__TimeOnline() { return offsetof(Player, TimeOnline); }; \


#define TestStrcuts_Source_h_43_PROPERTIES_GET \
public:\
virtual Reflect::ReflectMember GetMember(const std::string_view& memberName) override;\
virtual std::vector<Reflect::ReflectMember> GetMembers(std::vector<std::string> const& flags) override;\


#define TestStrcuts_Source_h_43_GENERATED_BODY \
TestStrcuts_Source_h_43_STATIC_CLASS \
TestStrcuts_Source_h_43_PROPERTIES \
TestStrcuts_Source_h_43_FUNCTION_DECLARE \
TestStrcuts_Source_h_43_FUNCTION_GET \
TestStrcuts_Source_h_43_PROPERTIES_OFFSET \
TestStrcuts_Source_h_43_PROPERTIES_GET \


#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID TestStrcuts_Source_h
