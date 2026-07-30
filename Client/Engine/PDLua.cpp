#include <Engine/PDLua.h>

void PDLua::initialize()
{
	m_lua.open_libraries(sol::lib::base, sol::lib::math);

	m_lua.new_usertype<PDPosition>("PDPosition", "x", &PDPosition::x, "y", &PDPosition::y);
	m_lua.new_usertype<PDVelocity>("PDVelocity", "x", &PDVelocity::x, "y", &PDVelocity::y);
}

sol::protected_function *PDLua::behavior(std::string const &scriptPath)
{
	auto const existing = m_behaviors.find(scriptPath);

	if (existing != m_behaviors.end())
	{
		return &existing->second;
	}

	sol::protected_function_result const result = m_lua.script_file(scriptPath, sol::script_pass_on_error);

	if (not result.valid())
	{
		return nullptr;
	}

	sol::protected_function function = m_lua["tick"];

	return &m_behaviors.emplace(scriptPath, std::move(function)).first->second;
}

void PDLua::tick(
	std::string const &scriptPath,
	PDPosition &position,
	PDVelocity &velocity,
	float deltaSeconds,
	float boundsWidth,
	float boundsHeight,
	float spriteWidth,
	float spriteHeight)
{
	sol::protected_function *const function = behavior(scriptPath);

	if (function == nullptr)
	{
		return;
	}

	(*function)(position, velocity, deltaSeconds, boundsWidth, boundsHeight, spriteWidth, spriteHeight);
}
