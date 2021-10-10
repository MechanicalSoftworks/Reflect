#pragma once

namespace Reflect
{
	const std::string ClassKey = "class";
	const std::string StructKey = "struct";
	const std::string ContainerKeys[] =
	{
		ClassKey, StructKey
	};

	const std::string TypedefKey = "typedef";
	const std::string UsingKey = "using";
	const std::string AliasKeys[] =
	{
		TypedefKey, UsingKey
	};

	const std::string PublicKey = "public";
	const std::string ProtectedKey = "protected";
	const std::string PrivateKey = "private";
	const std::string VisabilityKeys[] = 
	{
		PublicKey, ProtectedKey, PrivateKey
	};

	const std::string ReferenceKey = "&";
	const std::string PointerKey = "*";

	const std::string ConstKey = "const";
	const std::string StaticKey = "static";
	const std::string VolatileKey = "volatile";
	const std::string VirtualKey = "virtual";
	const std::string ValueModifierKeys[] =
	{
		ConstKey, StaticKey, VolatileKey, VirtualKey
	};

	const std::string PointerTypeIdKey = "__ptr64";
	const std::string TemplateKey = "template";
}