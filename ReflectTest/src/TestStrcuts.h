#pragma once

#include "Reflect.h"
#include "TestStrcuts.reflect.h"

#define EXPORT

/// <summary>
/// Example class.
/// </summary>

REFLECT_STRUCT()
struct S : REFLECT_BASE()
{
	REFLECT_GENERATED_BODY()

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
	REFLECT_GENERATED_BODY()

public:
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
	Player()
		: Id("PlayerExampleId")
	{ }

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
	std::string Id;

	REFLECT_PROPERTY(EditorOnly, Public)
		int Friends;
	REFLECT_PROPERTY(Public)
		int TimeOnline = 0;
};