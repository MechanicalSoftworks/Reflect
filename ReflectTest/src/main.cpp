#include <Reflect.h>
#include "TestStrcuts.h"
#include <iostream>
#include <cstdlib>

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
	Player player(Reflect::Constructor(Player::StaticClass, nullptr));
	auto playerGetId = player.GetFunction("PrintHelloWorld");
	std::cout << Reflect::ReflectReturnCodeToString(playerGetId.Invoke());
}

void FuncReturnValue()
{
	// Get a function with a return value std::string.
	// The return value with be set to playerId.
	Player player(Reflect::Constructor(Player::StaticClass, nullptr));
	Reflect::ReflectFunction playerGetId = player.GetFunction("GetId");
	std::string playerId;
	std::cout << Reflect::ReflectReturnCodeToString(playerGetId.Invoke(&playerId)) << ", Id = " << playerId << std::endl;
}

void FuncWithParameters()
{
	// Get a function with no return value but which has a single
	// parameter.
	Player player(Reflect::Constructor(Player::StaticClass, nullptr));
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
	Player player(Reflect::Constructor(Player::StaticClass, nullptr));
	auto member = player.GetMember("");
	auto membersWithPublic = player.GetMembers({ "Public" });
	int& friendInt = *membersWithPublic[1].ConvertToType<int>();
	friendInt = 12;
}

void StaticClass()
{
	const auto& staticClass = Reflect::Class::Lookup("Player");
	auto player = (Player*)staticClass.Allocator.Allocate();
	staticClass.Allocator.Construct(player, Reflect::Constructor(Player::StaticClass, nullptr));
	player->Tick();
	staticClass.Allocator.Destroy(player);
	staticClass.Allocator.Deallocate(player);
}

template<bool b> struct Baz;
template<> struct Baz<false>
{
	static constexpr int value = 0;
};
template<> struct Baz<true>
{
	static constexpr std::string_view value = "yes";
};

template<Reflect::Util::StringLiteral flag>
constexpr auto Bar()
{
	return std::get<2>(Reflect::ReflectStatic<Player>::Properties).HasAnyFlag<flag>();
}

template<Reflect::Util::StringLiteral flag>
auto Foo()
{
	constexpr auto x = Bar<flag>();
	return Baz<x>::value;
}

int main(void)
{
	{
		Player p(Reflect::Constructor(Player::StaticClass, nullptr));

		//
		// TODO: Add support for multiple arguments here.
		// TODO: Update the FilterProperties and ForEachProperty methods to use the templated filters.
		//
		auto x = Foo<"Serialise">();

		constexpr auto asdf = Reflect::FilterProperties<Player, "Serialise">();

		ForEachProperty(
			[](const std::string_view& name, auto&& arg) {
				std::cout << name << ": " << arg << std::endl;
			},
			p
		);

		ForEachProperty<"Serialise">(
			[](const std::string_view& name, auto&& arg) {
				std::cout << name << ": " << arg << std::endl;
			}, 
			p
		);
	}
	FuncNoReturn();
	FuncReturnValue();
	FuncWithParameters();
	GetMemberWithFlags();
	StaticClass();

	return 0;
}