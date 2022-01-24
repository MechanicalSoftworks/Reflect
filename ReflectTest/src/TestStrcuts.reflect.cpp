#include "TestStrcuts.h"

Reflect::ReflectMemberProp S::__REFLECT_MEMBER_PROPS__[2] = {
	Reflect::ReflectMemberProp("Friends", Reflect::Util::GetTypeName<int>(), __REFLECT__Friends(), {"EditorOnly", "Public"}),
	Reflect::ReflectMemberProp("TimeOnline", Reflect::Util::GetTypeName<int>(), __REFLECT__TimeOnline(), {"Public"}),
};

const Reflect::Class S::StaticClass = Reflect::Class("S", sizeof(S), alignof(S), nullptr, 2, __REFLECT_MEMBER_PROPS__);

Reflect::ReflectFunction S::GetFunction(const char* functionName)
{
	return SuperClass::GetFunction(functionName);
}

Reflect::ReflectMember S::GetMember(const char* memberName)
{
	for(const auto& member : __REFLECT_MEMBER_PROPS__)
	{
		if(memberName == member.Name)
		{
			//CheckFlags
			return Reflect::ReflectMember(member.Name, member.Type, ((char*)this) + member.Offset);
		}
	}
	return SuperClass::GetMember(memberName);
}

std::vector<Reflect::ReflectMember> S::GetMembers(std::vector<std::string> const& flags)
{
	std::vector<Reflect::ReflectMember> members = SuperClass::GetMembers(flags);
	for(auto& member : __REFLECT_MEMBER_PROPS__)
	{
		if(member.ContainsProperty(flags))
		{
			members.push_back(Reflect::ReflectMember(member.Name, member.Type, ((char*)this) + member.Offset));
		}
	}
	return members;
}

const Reflect::Class Actor::StaticClass = Reflect::Class("Actor", sizeof(Actor), alignof(Actor), nullptr, 0, nullptr);

Reflect::ReflectFunction Actor::GetFunction(const char* functionName)
{
	return SuperClass::GetFunction(functionName);
}

Reflect::ReflectMember Actor::GetMember(const char* memberName)
{
	return SuperClass::GetMember(memberName);
}

std::vector<Reflect::ReflectMember> Actor::GetMembers(std::vector<std::string> const& flags)
{
	std::vector<Reflect::ReflectMember> members = SuperClass::GetMembers(flags);
	return members;
}

Reflect::ReflectMemberProp Player::__REFLECT_MEMBER_PROPS__[2] = {
	Reflect::ReflectMemberProp("Friends", Reflect::Util::GetTypeName<int>(), __REFLECT__Friends(), {"EditorOnly", "Public"}),
	Reflect::ReflectMemberProp("TimeOnline", Reflect::Util::GetTypeName<int>(), __REFLECT__TimeOnline(), {"Public"}),
};

const Reflect::Class Player::StaticClass = Reflect::Class("Player", sizeof(Player), alignof(Player), &Actor::StaticClass, 2, __REFLECT_MEMBER_PROPS__);

Reflect::ReflectFunction Player::GetFunction(const char* functionName)
{
	if(functionName == "GetOnlineFriendsCount")
	{
		return Reflect::ReflectFunction(this, Player::__REFLECT_FUNC__GetOnlineFriendsCount);
	}
	if(functionName == "PrintHelloWorld")
	{
		return Reflect::ReflectFunction(this, Player::__REFLECT_FUNC__PrintHelloWorld);
	}
	if(functionName == "GetId")
	{
		return Reflect::ReflectFunction(this, Player::__REFLECT_FUNC__GetId);
	}
	return SuperClass::GetFunction(functionName);
}

Reflect::ReflectMember Player::GetMember(const char* memberName)
{
	for(const auto& member : __REFLECT_MEMBER_PROPS__)
	{
		if(memberName == member.Name)
		{
			//CheckFlags
			return Reflect::ReflectMember(member.Name, member.Type, ((char*)this) + member.Offset);
		}
	}
	return SuperClass::GetMember(memberName);
}

std::vector<Reflect::ReflectMember> Player::GetMembers(std::vector<std::string> const& flags)
{
	std::vector<Reflect::ReflectMember> members = SuperClass::GetMembers(flags);
	for(auto& member : __REFLECT_MEMBER_PROPS__)
	{
		if(member.ContainsProperty(flags))
		{
			members.push_back(Reflect::ReflectMember(member.Name, member.Type, ((char*)this) + member.Offset));
		}
	}
	return members;
}

