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

template<typename T> concept IsIReflect = requires(T t) { requires std::is_same_v<Reflect::IReflect, T>; };
template<typename T> concept IsNotIReflect = requires(T t) { requires !IsIReflect<T>; };

//
// This version has no filter.
//
#if 1

template<typename Callable, typename T> requires IsIReflect<T>
void ForEachProperty(Callable&& obj, T& t) {}

template<typename Callable, typename T> requires IsNotIReflect<T>
void ForEachProperty(Callable&& obj, T& t)
{
	// https://stackoverflow.com/a/54053084
	std::apply(
		[&t, &obj](auto&&... args) {
			((obj(args.Name, args.Get(t)), ...));
		},
		constexpr_reflect<T>::Properties
	);
	ForEachProperty<Callable, typename T::SuperClass>(std::move(obj), t);
}

#endif

//
// Filter by flags.
//
#if 1

template<typename T, typename TFlags, int I>
struct FilterPropertiesInternal;

template<typename T, typename TFlags>
struct FilterPropertiesInternal<T, TFlags, 0>
{
	static constexpr auto call(const TFlags& flags)
	{
		return std::make_tuple();
	}
};

template<typename T, typename TFlags, int I>
struct FilterPropertiesInternal
{
	static constexpr auto call(const TFlags& flags)
	{
		constexpr auto properties = constexpr_reflect<T>::Properties;
		constexpr auto p = std::get<I - 1>(properties);
		if constexpr (p.HasAnyFlag(flags))
		{
			return std::tuple_cat(
				std::make_tuple(p),
				FilterPropertiesInternal<T, TFlags, I - 1>::call(flags)
			);
		}
		else
		{
			return FilterPropertiesInternal<T, TFlags, I - 1>::call(flags);
		}
	}
};

template<typename T, typename TFlags>
constexpr auto FilterProperties(const TFlags& flags)
{
	constexpr auto properties = constexpr_reflect<T>::Properties;

	constexpr auto i = std::tuple_size<decltype(properties)>::value;
	return FilterPropertiesInternal<T, TFlags, i>::call(flags);
}

template<typename Callable, typename T, typename TFlags> requires IsIReflect<T>
void ForEachProperty(Callable&& obj, T& t, const TFlags& flags) {}

template<typename Callable, typename T, typename TFlags> requires IsNotIReflect<T>
void ForEachProperty(Callable&& obj, T& t, TFlags flags)
{
	constexpr auto properties = FilterProperties<T>(flags);

	// https://stackoverflow.com/a/54053084
	std::apply(
		[&t, &obj](auto&&... args) {
			((obj(args.Name, args.Get(t)), ...));
		},
		properties
	);

	ForEachProperty<Callable, typename T::SuperClass>(std::move(obj), t, flags);
}
#endif

#if 0
template<typename T, size_t N>
class constexpr_string
{
	using ContainerType = std::array<T, N>;

public:
	constexpr constexpr_string(const ContainerType& a) : Buffer(a) {}

	constexpr auto view() const { return std::string_view(Buffer.data(), N); }

private:
	const ContainerType	Buffer;
};

constexpr auto constexpr_reflect()
{
	constexpr std::string_view test("test");
	return constexpr_string(
		Reflect::Util::detail::impl::substring_as_array(test, std::make_index_sequence<test.length()>{})
	);
}
#endif

int main(void)
{
	{
		Player p(Reflect::Constructor(Player::StaticClass, nullptr));

		constexpr auto asdf = FilterProperties<Player>(std::make_tuple("Serialise"));

		ForEachProperty(
			[](const std::string_view& name, auto&& arg) {
				std::cout << name << ": " << arg << std::endl;
			}, 
			p,
			std::make_tuple("Serialise")
		);
	}
	FuncNoReturn();
	FuncReturnValue();
	FuncWithParameters();
	GetMemberWithFlags();
	StaticClass();

	return 0;
}