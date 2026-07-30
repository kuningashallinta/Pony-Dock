#pragma once

#include <Engine/Components/PDPosition.h>
#include <Engine/Components/PDVelocity.h>

#include <sol/sol.hpp>

#include <string>
#include <unordered_map>

class PDLua
{
public:
	void initialize();

	void tick(
		std::string const &scriptPath,
		PDPosition &position,
		PDVelocity &velocity,
		float deltaSeconds,
		float boundsWidth,
		float boundsHeight,
		float spriteWidth,
		float spriteHeight);

private:
	sol::protected_function *behavior(std::string const &scriptPath);

	sol::state m_lua;
	std::unordered_map<std::string, sol::protected_function> m_behaviors;
};
