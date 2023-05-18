#pragma once

#include <Reflect.h>
#include "TestStrcuts.reflect.h"

#define EXPORT

REFLECT_ENUM(ValueType = int)
class E : public Reflect::IEnum
{
public:
	enum Values
	{
		Value1,
		// hello world
		Value2,

		Value3 = 10,
		Value4
	};

	REFLECT_GENERATED_BODY();
};

/// <summary>
/// Example class.
/// </summary>

REFLECT_STRUCT()
struct S : REFLECT_BASE()
{
	REFLECT_GENERATED_BODY()

	using SuperClass::SuperClass;

	REFLECT_PROPERTY(EditorOnly, Public)
	int Friends;
	REFLECT_PROPERTY(Public)
	int TimeOnline = 0;
};

template<typename T> struct constexpr_reflect;

class C
{

};

REFLECT_CLASS()
class EXPORT Actor : REFLECT_BASE()
{
	friend class constexpr_reflect<Actor>;
	REFLECT_GENERATED_BODY()

public:
	using SuperClass::SuperClass;

	virtual void Tick()
	{
		printf("Actor::Tick");
	}
};

REFLECT_CLASS(AllPrivate, ShowInEditorOnly, EditorOnly)
class EXPORT Player : public Actor
{
	friend class constexpr_reflect<Player>;
	REFLECT_GENERATED_BODY()

public:
	Player(const Reflect::Constructor& init) 
		: SuperClass(init)
		, Id("PlayerExampleId")
		, Friends(7)
		, TimeOnline(24)
	{}

	~Player()
	{ }

	REFLECT_PROPERTY(Public)
		int GetOnlineFriendsCount(int const& maxPlayerCount);

	REFLECT_PROPERTY()
		void PrintHelloWorld();

	void Tick() override
	{
		printf("Player::Tick");
	}

private:
	REFLECT_PROPERTY()
		std::string GetId() const;

private:
	REFLECT_PROPERTY(EditorOnly, Public)
		std::string Id;

	REFLECT_PROPERTY(EditorOnly, Public)
		int Friends;
	REFLECT_PROPERTY(Public)
		int TimeOnline = 0;
};

template <typename T, T... S, typename F>
constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f) {
	using unpack_t = int[];
	(void)unpack_t {
		(static_cast<void>(f(std::integral_constant<T, S>{})), 0)..., 0
	};
}

// https://www.fluentcpp.com/2019/03/08/stl-algorithms-on-tuples/
template <class Tuple, class F, std::size_t... I>
constexpr F for_each_impl(Tuple&& t, F&& f, std::index_sequence<I...>)
{
	return (void)std::initializer_list<int>{(std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))), 0)...}, f;
}

// https://www.fluentcpp.com/2019/03/08/stl-algorithms-on-tuples/
template <class Tuple, class F>
constexpr F for_each(Tuple&& t, F&& f)
{
	return for_each_impl(std::forward<Tuple>(t), std::forward<F>(f),
		std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}

// https://www.fluentcpp.com/2019/03/08/stl-algorithms-on-tuples/
template<typename Tuple, typename Predicate>
constexpr size_t find_if(Tuple&& tuple, Predicate pred)
{
	size_t index = std::tuple_size<std::remove_reference_t<Tuple>>::value;
	size_t currentIndex = 0;
	bool found = false;
	for_each(tuple, [&](auto&& value)
		{
			if (!found && pred(value))
			{
				index = currentIndex;
				found = true;
			}
			++currentIndex;
		});
	return index;
}

template<typename Tuple, typename Predicate>
constexpr bool all_of(Tuple&& tuple, Predicate pred)
{
	return find_if(tuple, std::not_fn(pred)) == std::tuple_size<std::decay_t<Tuple>>::value;
}

template<typename Tuple, typename Predicate>
constexpr bool none_of(Tuple&& tuple, Predicate pred)
{
	return find_if(tuple, pred) == std::tuple_size<std::decay_t<Tuple>>::value;
}

template<typename Tuple, typename Predicate>
constexpr bool any_of(Tuple&& tuple, Predicate pred)
{
	return !none_of(tuple, pred);
}

template<typename F, typename TAttributes>
struct CompileTimeField
{
	using Type = F;
	using AttributesType = TAttributes;

	constexpr CompileTimeField(const char* name, std::size_t offset, TAttributes attributes)
		: Name(name), Offset(offset), Attributes(attributes)
	{}

	const char* const	Name;
	const std::size_t	Offset;
	const TAttributes	Attributes;

	template<typename T>
	auto& Get(T& t) const
	{
		return *reinterpret_cast<Type*>(reinterpret_cast<char*>(&t) + Offset);
	}

	template<typename TFlags>
	constexpr auto HasAnyFlag(const TFlags& flags) const
	{
		bool found = false;

		for_each(flags, [&](auto&& value)
		{
			found = found || any_of(Attributes, [&](const auto& x) { return x == value; });
		});

		return found;
	}
};

template<typename F>
constexpr auto make_compile_time_field(const char* name, std::size_t offset, auto attributes)
{
	return CompileTimeField<F, decltype(attributes)>(name, offset, attributes);
}

template<>
struct constexpr_reflect<Actor>
{
	static inline constexpr auto Properties = std::make_tuple();
};

template<>
struct constexpr_reflect<Player>
{
	// TODO: Attributes as a tuple.
	static inline constexpr auto Properties = std::make_tuple(
		make_compile_time_field<std::string>("Id", Player::__OFFSETOF__Id(), std::make_tuple()),
		make_compile_time_field<int>("Friends", Player::__OFFSETOF__Friends(), std::make_tuple()),
		make_compile_time_field<int>("TimeOnline", Player::__OFFSETOF__TimeOnline(), std::make_tuple("Serialise"))
	);
};