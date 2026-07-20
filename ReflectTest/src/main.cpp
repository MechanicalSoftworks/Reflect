#include <Reflect.h>
#include "TestStrcuts.h"
#include <iostream>
#include <cstdlib>
#include <memory_resource>

#ifdef _MSC_VER
void* aligned_alloc(std::size_t size, std::size_t alignment) { return _aligned_malloc(size, alignment); }
void aligned_free(void* p) noexcept { _aligned_free(p); }
#else
void* aligned_alloc(std::size_t size, std::size_t alignment) { return std::aligned_alloc(size, alignment); }
void aligned_free(void* p) noexcept { std::free(p); }
#endif

void FuncNoReturn()
{
	// Get a function with no return value.
	Player player(Reflect::Constructor(*std::pmr::get_default_resource(), nullptr));
	auto playerGetId = player.GetFunction("PrintHelloWorld");
	std::cout << Reflect::ReflectReturnCodeToString(playerGetId.Invoke());
}

void FuncReturnValue()
{
	// Get a function with a return value std::string.
	// The return value with be set to playerId.
	Player player(Reflect::Constructor(*std::pmr::get_default_resource(), nullptr));
	Reflect::ReflectFunction playerGetId = player.GetFunction("GetId");
	std::string playerId;
	std::cout << Reflect::ReflectReturnCodeToString(playerGetId.Invoke(&playerId)) << ", Id = " << playerId << std::endl;
}

void FuncWithParameters()
{
	// Get a function with no return value but which has a single
	// parameter.
	Player player(Reflect::Constructor(*std::pmr::get_default_resource(), nullptr));
	Reflect::ReflectFunction parameterFunc = player.GetFunction("GetOnlineFriendsCount");
	
	// Setup the parameter to send to the function. This is order
	// sensitive.
	Reflect::FunctionPtrArgs args;
	int intParameter = 8;
	args.AddArg(&intParameter);

	int returnCount = -1;
	std::cout << Reflect::ReflectReturnCodeToString(parameterFunc.Invoke(&returnCount, args)) << ", FriendsCount = " << returnCount << std::endl;
}

void GetMemberWithFlags()
{
	Player player(Reflect::Constructor(*std::pmr::get_default_resource(), nullptr));
	auto member = player.GetMember("");
	auto membersWithPublic = player.GetMembers({ "Public" }, *std::pmr::get_default_resource());
	int& friendInt = *membersWithPublic[1].ConvertToType<int>();
	friendInt = 12;
}

void StaticClass()
{
	const auto& staticClass = Reflect::Class::Lookup("Player");
	auto player = (Player*)staticClass.Allocator.New(
		Reflect::Constructor(*std::pmr::get_default_resource(), nullptr)
	);
	player->Tick();
	staticClass.Allocator.Delete(*std::pmr::get_default_resource(), player);
}

int main(void)
{
	static_assert(Reflect::Util::GetFunctionName<&FuncWithParameters>() == "FuncWithParameters");
	static_assert(Reflect::Util::GetFunctionName<&C::operator()>() == "C::operator()");
	static_assert(Reflect::Util::GetFunctionDeclaration<&C::operator()>() == "void C::operator()()");
	static_assert(Reflect::Util::GetFunctionDeclaration<&Player::GetOnlineFriendsCount>() == "int32 Player::GetOnlineFriendsCount(const int&)");

	static_assert(Reflect::Util::GetTypeName<Actor>() == "Actor");
	static_assert(Reflect::Util::GetTypeName<bool>() == "bool");
	static_assert(Reflect::Util::GetTypeName<int>() == "int32");
	static_assert(Reflect::Util::GetTypeName<std::vector<int>>() == "std::vector<int32>");

	{
		Player p(Reflect::Constructor(*std::pmr::get_default_resource(), nullptr));

		ForEachProperty(p,
			[]<typename F, typename TAttributes>(const Reflect::StaticField<F, TAttributes>& property, F& arg) {
				std::cout << property.Name << ": " << arg << std::endl;
			}
		);

		ForEachProperty<"Serialise">(p,
			[]<typename F, typename TAttributes>(const Reflect::StaticField<F, TAttributes>& property, F& arg) {
				std::cout << property.Name << ": " << arg << std::endl;
			}
		);
	}
	FuncNoReturn();
	FuncReturnValue();
	FuncWithParameters();
	GetMemberWithFlags();
	StaticClass();

	return 0;
}