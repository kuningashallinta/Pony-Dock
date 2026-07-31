#include <Engine/PDLua.h>

#include <Engine/PDDiagnostics.h>

#include <algorithm>
#include <filesystem>

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
	sol::reference const traceback = m_lua["debug"]["traceback"];
	sol::object const tickObject = moduleTable["tick"];

	if (tickObject.is<sol::protected_function>())
	{
		created.tick = sol::protected_function(tickObject.as<sol::protected_function>(), traceback);
	}

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

	if (not target->tick.valid())
	{
		m_diagnostics->writeOnce(scriptPath + ":tick", "Script has no tick function: " + scriptPath);

		return false;
	}

	return report(scriptPath, target->tick(self, deltaSeconds));
}

bool PDLua::loadScript(std::string const &scriptPath)
{
	auto const existing = m_modules.find(scriptPath);

	if (existing != m_modules.end() and existing->second.failed)
	{
		m_modules.erase(existing);
	}

	return module(scriptPath) != nullptr;
}

void PDLua::unloadScript(std::string const &scriptPath)
{
	std::string const normalized = std::filesystem::path(scriptPath).lexically_normal().string();

	for (auto iterator = m_modules.begin(); iterator != m_modules.end();)
	{
		if (std::filesystem::path(iterator->first).lexically_normal().string() == normalized)
		{
			iterator = m_modules.erase(iterator);
		}
		else
		{
			++iterator;
		}
	}

	forgetPackage(normalized);
	m_diagnostics->write("Unloaded script " + normalized);
}

void PDLua::reload()
{
	m_modules.clear();
	forgetPackage(std::string());
}

std::string PDLua::modulePath(std::string const &name) const
{
	std::string const candidates[] = {
		m_scriptsRoot + "/" + name + ".lua",
		m_scriptsRoot + "/lib/" + name + ".lua",
	};

	for (std::string const &candidate : candidates)
	{
		std::error_code error;

		if (std::filesystem::exists(candidate, error))
		{
			return std::filesystem::path(candidate).lexically_normal().string();
		}
	}

	return std::string();
}

void PDLua::forgetPackage(std::string const &normalizedPath)
{
	sol::optional<sol::table> loaded = m_lua["package"]["loaded"];

	if (not loaded.has_value())
	{
		return;
	}

	std::vector<std::string> names;

	for (auto const &entry : loaded.value())
	{
		sol::optional<std::string> const name = entry.first.as<sol::optional<std::string>>();

		if (not name.has_value())
		{
			continue;
		}

		std::string const path = modulePath(name.value());

		if (path.empty())
		{
			continue;
		}

		if (normalizedPath.empty() or path == normalizedPath)
		{
			names.push_back(name.value());
		}
	}

	for (std::string const &name : names)
	{
		loaded.value()[name] = sol::nil;
	}
}

std::vector<std::string> PDLua::loadedScripts() const
{
	std::vector<std::string> result;

	for (auto const &entry : m_modules)
	{
		if (not entry.second.failed)
		{
			result.push_back(std::filesystem::path(entry.first).lexically_normal().string());
		}
	}

	sol::optional<sol::table> const loaded = m_lua["package"]["loaded"];

	if (loaded.has_value())
	{
		for (auto const &entry : loaded.value())
		{
			sol::optional<std::string> const name = entry.first.as<sol::optional<std::string>>();

			if (not name.has_value())
			{
				continue;
			}

			std::string const path = modulePath(name.value());

			if (not path.empty())
			{
				result.push_back(path);
			}
		}
	}

	std::sort(result.begin(), result.end());
	result.erase(std::unique(result.begin(), result.end()), result.end());

	return result;
}
