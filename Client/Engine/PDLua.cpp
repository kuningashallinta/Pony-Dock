#include <Engine/PDLua.h>

#include <Engine/PDDiagnostics.h>

void PDLua::initialize(std::string const &scriptsRoot, PDDiagnostics &diagnostics)
{
	m_diagnostics = &diagnostics;
	m_scriptsRoot = scriptsRoot;

	m_lua.open_libraries(
		sol::lib::base,
		sol::lib::math,
		sol::lib::string,
		sol::lib::table,
		sol::lib::package,
		sol::lib::debug);

	m_lua["package"]["path"] = scriptsRoot + "/?.lua;" + scriptsRoot + "/lib/?.lua";

	m_frame = m_lua.create_table();
	m_frame["screen"] = m_lua.create_table();
	m_lua["PD"] = m_frame;

	m_metatable = m_lua.create_table();

	m_metatable["play"] = [this](sol::table self, std::string const &name, sol::optional<bool> loop)
	{
		if (not m_playHandler)
		{
			return false;
		}

		return m_playHandler(self["id"].get<std::uint32_t>(), name, loop.value_or(true), false);
	};

	m_metatable["replay"] = [this](sol::table self, std::string const &name, sol::optional<bool> loop)
	{
		if (not m_playHandler)
		{
			return false;
		}

		return m_playHandler(self["id"].get<std::uint32_t>(), name, loop.value_or(true), true);
	};

	m_metatable["set_facing"] = [this](sol::table self, std::string const &facing)
	{
		if (not m_facingHandler)
		{
			return;
		}

		m_facingHandler(self["id"].get<std::uint32_t>(), facing != "left");
	};

	m_metatable["log"] = [this](sol::table self, std::string const &message)
	{
		m_diagnostics->write("[entity " + std::to_string(self["id"].get<std::uint32_t>()) + "] " + message);
	};

	m_metatable["__index"] = m_metatable;
}

void PDLua::setPlayHandler(PlayHandler handler)
{
	m_playHandler = std::move(handler);
}

void PDLua::setFacingHandler(FacingHandler handler)
{
	m_facingHandler = std::move(handler);
}

void PDLua::beginFrame(float boundsWidth, float boundsHeight)
{
	sol::table screen = m_frame["screen"];
	screen["width"] = boundsWidth;
	screen["height"] = boundsHeight;
}

sol::table PDLua::createSelf(std::uint32_t entityId)
{
	sol::table self = m_lua.create_table();
	self["id"] = entityId;
	self[sol::metatable_key] = m_metatable;

	return self;
}

sol::table PDLua::packTable(PDPonyPackData const &pack)
{
	auto const existing = m_packTables.find(pack.packPath);

	if (existing != m_packTables.end())
	{
		return existing->second;
	}

	sol::table behaviors = m_lua.create_table();
	sol::table byId = m_lua.create_table();

	for (std::size_t index = 0; index < pack.behaviors.size(); index += 1)
	{
		PDPonyBehaviorData const &source = pack.behaviors[index];

		sol::table behavior = m_lua.create_table();
		behavior["id"] = source.id;
		behavior["name"] = source.name;
		behavior["animation"] = source.animation;
		behavior["movement"] = source.movement;
		behavior["chance"] = source.chance;
		behavior["duration_min"] = source.durationMinSeconds;
		behavior["duration_max"] = source.durationMaxSeconds;
		behavior["speed"] = source.speedPxPerSec;
		behavior["group"] = source.group;
		behavior["special"] = source.special;
		behavior["skip"] = source.skip;
		behavior["prevent_animation_loop"] = source.preventAnimationLoop;

		if (not source.linkedBehavior.empty())
		{
			behavior["linked"] = source.linkedBehavior;
		}

		behaviors[index + 1] = behavior;

		if (not source.id.empty())
		{
			byId[source.id] = behavior;
		}
	}

	sol::table real = m_lua.create_table();
	real["id"] = pack.id;
	real["behaviors"] = behaviors;
	real["by_id"] = byId;

	sol::table proxy = m_lua.create_table();
	sol::table meta = m_lua.create_table();
	meta["__index"] = real;
	meta["__metatable"] = false;
	meta["__newindex"] = [](sol::this_state state, sol::table, sol::object, sol::object)
	{
		return luaL_error(state.lua_state(), "pack data is read-only");
	};

	proxy[sol::metatable_key] = meta;

	return m_packTables.emplace(pack.packPath, proxy).first->second;
}

PDLua::Module *PDLua::module(std::string const &scriptPath)
{
	auto const existing = m_modules.find(scriptPath);

	if (existing != m_modules.end())
	{
		return existing->second.failed ? nullptr : &existing->second;
	}

	Module created;
	created.environment = sol::environment(m_lua, sol::create, m_lua.globals());

	sol::protected_function_result const result = m_lua.script_file(scriptPath, created.environment, sol::script_pass_on_error);

	if (not result.valid())
	{
		sol::error const error = result;
		m_diagnostics->writeOnce(scriptPath, "Script load failed: " + std::string(error.what()));
		created.failed = true;
		m_modules.emplace(scriptPath, std::move(created));

		return nullptr;
	}

	sol::object const returned = result;

	if (not returned.is<sol::table>())
	{
		m_diagnostics->writeOnce(scriptPath, "Script did not return a table: " + scriptPath);
		created.failed = true;
		m_modules.emplace(scriptPath, std::move(created));

		return nullptr;
	}

	sol::table const moduleTable = returned.as<sol::table>();
	sol::object const tickObject = moduleTable["tick"];

	if (not tickObject.is<sol::protected_function>())
	{
		m_diagnostics->writeOnce(scriptPath, "Script has no tick function: " + scriptPath);
		created.failed = true;
		m_modules.emplace(scriptPath, std::move(created));

		return nullptr;
	}

	sol::reference const traceback = m_lua["debug"]["traceback"];
	created.tick = sol::protected_function(tickObject.as<sol::protected_function>(), traceback);

	sol::object const spawnObject = moduleTable["spawn"];

	if (spawnObject.is<sol::protected_function>())
	{
		created.spawn = sol::protected_function(spawnObject.as<sol::protected_function>(), traceback);
	}

	m_diagnostics->write("Loaded script " + scriptPath);

	return &m_modules.emplace(scriptPath, std::move(created)).first->second;
}

bool PDLua::report(std::string const &scriptPath, sol::protected_function_result const &result)
{
	if (result.valid())
	{
		return true;
	}

	sol::error const error = result;
	m_diagnostics->writeOnce(scriptPath + ":runtime", "Script error in " + scriptPath + ": " + error.what());

	return false;
}

bool PDLua::callSpawn(std::string const &scriptPath, sol::table &self)
{
	Module *const target = module(scriptPath);

	if (target == nullptr)
	{
		return false;
	}

	if (not target->spawn.valid())
	{
		return true;
	}

	return report(scriptPath, target->spawn(self));
}

bool PDLua::callTick(std::string const &scriptPath, sol::table &self, float deltaSeconds)
{
	Module *const target = module(scriptPath);

	if (target == nullptr)
	{
		return false;
	}

	return report(scriptPath, target->tick(self, deltaSeconds));
}

void PDLua::reload()
{
	m_modules.clear();
}
