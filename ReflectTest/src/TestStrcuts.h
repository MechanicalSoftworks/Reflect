#pragma once

#include <Reflect.h>
#include <ReflectStatic.h>
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

class C
{

};

REFLECT_CLASS()
class EXPORT Actor : REFLECT_BASE()
{
	friend class Reflect::ReflectStatic<Actor>;
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
	friend class Reflect::ReflectStatic<Player>;
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

template<>
struct Reflect::ReflectStatic<Actor>
{
	static inline constexpr auto Properties = std::make_tuple();
};

template<>
struct Reflect::ReflectStatic<Player>
{
	// TODO: Attributes as a tuple.
	static inline constexpr auto Properties = std::make_tuple(
		Reflect::make_static_field<std::string>("Id", Player::__OFFSETOF__Id(), std::make_tuple()),
		Reflect::make_static_field<int>("Friends", Player::__OFFSETOF__Friends(), std::make_tuple()),
		Reflect::make_static_field<int>("TimeOnline", Player::__OFFSETOF__TimeOnline(), std::make_tuple("Serialise"))
	);
};