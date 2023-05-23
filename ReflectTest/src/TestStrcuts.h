#pragma once

#include <Reflect.h>
#include <ReflectStatic.h>
#include "TestStrcuts.Reflect.h"

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

REFLECT_CLASS(Abstract)
template<typename T>
class EXPORT TemplatedClass : REFLECT_BASE()
{
	REFLECT_GENERATED_BODY()

public:
	using SuperClass::SuperClass;

	REFLECT_PROPERTY(EditorOnly, Public)
		T Property;
};

REFLECT_CLASS()
class EXPORT Actor : REFLECT_BASE()
{
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

#include "TestStrcuts.ReflectStatic.h"