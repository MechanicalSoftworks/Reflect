#include "TestStrcuts.h"
#include "Core/Util.h"

Reflect::ReflectMemberProp S::__REFLECT_MEMBER_PROPS__[2] = {
	Reflect::CreateReflectMemberProp<int>("Friends", Reflect::Util::GetTypeName<int>(), __REFLECT__Friends(), {"EditorOnly", "Public"}, nullptr, nullptr),
	Reflect::CreateReflectMemberProp<int>("TimeOnline", Reflect::Util::GetTypeName<int>(), __REFLECT__TimeOnline(), {"Public"}, nullptr, nullptr),
};

const Reflect::Class S::StaticClass = Reflect::Class("S", nullptr, {}, 2, __REFLECT_MEMBER_PROPS__, Reflect::ClassAllocator::Create<S>());

Reflect::ReflectFunction S::GetFunction(const std::string_view &functionName) const
{
	return SuperClass::GetFunction(functionName);
}

Reflect::ReflectMember S::GetMember(const std::string_view& memberName) const
{
	for(const auto& member : __REFLECT_MEMBER_PROPS__)
	{
		if(memberName == member.Name)
		{
			return Reflect::ReflectMember(&member, ((char*)this) + member.Offset);
		}
	}
	return SuperClass::GetMember(memberName);
}

std::vector<Reflect::ReflectMember> S::GetMembers(std::vector<std::string> const& flags) const
{
	std::vector<Reflect::ReflectMember> members = SuperClass::GetMembers(flags);
	for(auto& member : __REFLECT_MEMBER_PROPS__)
	{
		if(member.ContainsProperty(flags))
		{
			members.push_back(Reflect::ReflectMember(&member, ((char*)this) + member.Offset));
		}
	}
	return members;
}

const Reflect::Class Actor::StaticClass = Reflect::Class("Actor", nullptr, {}, 0, nullptr, Reflect::ClassAllocator::Create<Actor>());

Reflect::ReflectFunction Actor::GetFunction(const std::string_view &functionName) const
{
	return SuperClass::GetFunction(functionName);
}

Reflect::ReflectMember Actor::GetMember(const std::string_view& memberName) const
{
	return SuperClass::GetMember(memberName);
}

std::vector<Reflect::ReflectMember> Actor::GetMembers(std::vector<std::string> const& flags) const
{
	std::vector<Reflect::ReflectMember> members = SuperClass::GetMembers(flags);
	return members;
}

Reflect::ReflectMemberProp Player::__REFLECT_MEMBER_PROPS__[2] = {
	Reflect::CreateReflectMemberProp<int>("Friends", Reflect::Util::GetTypeName<int>(), __REFLECT__Friends(), {"EditorOnly", "Public"}, nullptr, nullptr),
	Reflect::CreateReflectMemberProp<int>("TimeOnline", Reflect::Util::GetTypeName<int>(), __REFLECT__TimeOnline(), {"Public"}, nullptr, nullptr),
};

const Reflect::Class Player::StaticClass = Reflect::Class("Player", &Actor::StaticClass, {}, 2, __REFLECT_MEMBER_PROPS__, Reflect::ClassAllocator::Create<Player>());

Reflect::ReflectFunction Player::GetFunction(const std::string_view &functionName) const
{
	if(functionName == "GetOnlineFriendsCount")
	{
		return Reflect::ReflectFunction(const_cast<Player*>(this), Player::__REFLECT_FUNC__GetOnlineFriendsCount);
	}
	if(functionName == "PrintHelloWorld")
	{
		return Reflect::ReflectFunction(const_cast<Player*>(this), Player::__REFLECT_FUNC__PrintHelloWorld);
	}
	if(functionName == "GetId")
	{
		return Reflect::ReflectFunction(const_cast<Player*>(this), Player::__REFLECT_FUNC__GetId);
	}
	return SuperClass::GetFunction(functionName);
}

Reflect::ReflectMember Player::GetMember(const std::string_view& memberName) const
{
	for(const auto& member : __REFLECT_MEMBER_PROPS__)
	{
		if(memberName == member.Name)
		{
			return Reflect::ReflectMember(&member, ((char*)this) + member.Offset);
		}
	}
	return SuperClass::GetMember(memberName);
}

std::vector<Reflect::ReflectMember> Player::GetMembers(std::vector<std::string> const& flags) const
{
	std::vector<Reflect::ReflectMember> members = SuperClass::GetMembers(flags);
	for(auto& member : __REFLECT_MEMBER_PROPS__)
	{
		if(member.ContainsProperty(flags))
		{
			members.push_back(Reflect::ReflectMember(&member, ((char*)this) + member.Offset));
		}
	}
	return members;
}

