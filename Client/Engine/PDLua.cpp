#include <Engine/PDLua.h>

#include <Engine/PDDiagnostics.h>

#include <algorithm>
#include <filesystem>

void PDLua::initialize(std::string const &scriptsRoot, PDDiagnostics &diagnostics, PDSettingsStore &settings)
{
	m_diagnostics = &diagnostics;
	m_scriptsRoot = scriptsRoot;
	m_settings = &settings;

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
	m_frame["monitors"] = m_lua.create_table();
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

void PDLua::beginFrame(float boundsWidth, float boundsHeight, std::vector<PDRect> const &monitors)
{
	m_settings->refresh(m_snapshot);

	sol::table screen = m_frame["screen"];
	screen["width"] = boundsWidth;
	screen["height"] = boundsHeight;

	if (m_monitors == monitors)
	{
		return;
	}

	m_monitors = monitors;
	sol::table areas = m_lua.create_table();

	for (std::size_t index = 0; index < monitors.size(); index += 1)
	{
		sol::table area = m_lua.create_table();
		area["x"] = monitors[index].x;
		area["y"] = monitors[index].y;
		area["width"] = monitors[index].width;
		area["height"] = monitors[index].height;
		areas[index + 1] = area;
	}

	m_frame["monitors"] = areas;
}

sol::table PDLua::createSelf(std::uint32_t entityId, std::string const &scriptPath, std::string const &packId)
{
	std::string const key = PDSettingsStore::moduleKey(m_scriptsRoot, scriptPath);

	sol::table self = m_lua.create_table();
	self["id"] = entityId;

	self["setting"] = [this, key, packId](sol::this_state state, sol::table, std::string const &id)
	{
		return settingObject(state, key, packId, id);
	};

	self[sol::metatable_key] = m_metatable;

	return self;
}

sol::table PDLua::buildPackReal(PDPonyPackData const &pack)
{
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

	sol::table groups = m_lua.create_table();

	for (std::size_t index = 0; index < pack.groups.size(); index += 1)
	{
		groups[index + 1] = pack.groups[index].id;
	}

	sol::table real = m_lua.create_table();
	real["id"] = pack.id;
	real["behaviors"] = behaviors;
	real["by_id"] = byId;
	real["groups"] = groups;

	return real;
}

sol::table PDLua::packTable(PDPonyPackData const &pack)
{
	auto const existing = m_packTables.find(pack.packPath);

	if (existing != m_packTables.end())
	{
		return existing->second.proxy;
	}

	PackTables tables;
	tables.real = buildPackReal(pack);
	tables.meta = m_lua.create_table();
	tables.meta["__index"] = tables.real;
	tables.meta["__metatable"] = false;

	tables.meta["__newindex"] = [](sol::this_state state, sol::table, sol::object, sol::object)
	{
		return luaL_error(state.lua_state(), "pack data is read-only");
	};

	tables.proxy = m_lua.create_table();
	tables.proxy[sol::metatable_key] = tables.meta;

	return m_packTables.emplace(pack.packPath, std::move(tables)).first->second.proxy;
}

void PDLua::refreshPackTable(PDPonyPackData const &pack)
{
	auto const existing = m_packTables.find(pack.packPath);

	if (existing == m_packTables.end())
	{
		return;
	}

	existing->second.real = buildPackReal(pack);
	existing->second.meta["__index"] = existing->second.real;
}

bool PDLua::declareSetting(std::string const &moduleKey, PDSettingDeclaration declaration)
{
	std::string warning;
	bool const accepted = m_settings->declare(moduleKey, std::move(declaration), warning);

	if (not warning.empty())
	{
		m_diagnostics->writeOnce(moduleKey + ":" + warning, moduleKey + ": " + warning);
	}

	return accepted;
}

PDSettingValue PDLua::settingValue(std::string const &moduleKey, std::string const &packId, std::string const &id) const
{
	auto const module = m_snapshot.modules.find(moduleKey);

	if (module == m_snapshot.modules.end())
	{
		return {};
	}

	if (not packId.empty())
	{
		auto const pack = module->second.packs.find(packId);

		if (pack != module->second.packs.end())
		{
			auto const overridden = pack->second.find(id);

			if (overridden != pack->second.end())
			{
				return overridden->second;
			}
		}
	}

	auto const stored = module->second.values.find(id);

	if (stored != module->second.values.end())
	{
		return stored->second;
	}

	return {};
}

sol::object PDLua::settingObject(sol::this_state state, std::string const &moduleKey, std::string const &packId, std::string const &id) const
{
	PDSettingValue const value = settingValue(moduleKey, packId, id);

	switch (value.type)
	{
		case PDSettingValueType::Boolean:
		{
			return sol::make_object(state, value.boolean);
		}

		case PDSettingValueType::Number:
		{
			return sol::make_object(state, value.number);
		}

		case PDSettingValueType::Text:
		{
			return sol::make_object(state, value.text);
		}

		default:
		{
			return sol::make_object(state, sol::lua_nil);
		}
	}
}

sol::table PDLua::createSettingsTable(std::string const &moduleKey)
{
	sol::table table = m_lua.create_table();

	table["checkbox"] = [this, moduleKey](std::string const &id, std::string const &label, sol::optional<bool> value)
	{
		PDSettingDeclaration declaration;
		declaration.id = id;
		declaration.label = label;
		declaration.kind = PDSettingKind::Checkbox;
		declaration.defaultValue.type = PDSettingValueType::Boolean;
		declaration.defaultValue.boolean = value.value_or(false);

		declareSetting(moduleKey, std::move(declaration));
	};

	table["slider"] = [this, moduleKey](
						  std::string const &id,
						  std::string const &label,
						  float value,
						  float minimum,
						  float maximum)
	{
		PDSettingDeclaration declaration;
		declaration.id = id;
		declaration.label = label;
		declaration.kind = PDSettingKind::Slider;
		declaration.defaultValue.type = PDSettingValueType::Number;
		declaration.defaultValue.number = value;
		declaration.minimum = minimum;
		declaration.maximum = maximum;

		declareSetting(moduleKey, std::move(declaration));
	};

	table["text"] = [this, moduleKey](std::string const &id, std::string const &label, sol::optional<std::string> value)
	{
		PDSettingDeclaration declaration;
		declaration.id = id;
		declaration.label = label;
		declaration.kind = PDSettingKind::Text;
		declaration.defaultValue.type = PDSettingValueType::Text;
		declaration.defaultValue.text = value.value_or(std::string());

		declareSetting(moduleKey, std::move(declaration));
	};

	table["dropdown"] = [this, moduleKey](
							std::string const &id,
							std::string const &label,
							sol::table options,
							sol::optional<std::string> value)
	{
		PDSettingDeclaration declaration;
		declaration.id = id;
		declaration.label = label;
		declaration.kind = PDSettingKind::Dropdown;

		for (std::size_t index = 1; index <= options.size(); index += 1)
		{
			sol::optional<std::string> const option = options[index];

			if (option.has_value())
			{
				declaration.options.push_back(option.value());
			}
		}

		declaration.defaultValue.type = PDSettingValueType::Text;

		if (value.has_value())
		{
			declaration.defaultValue.text = value.value();
		}
		else if (not declaration.options.empty())
		{
			declaration.defaultValue.text = declaration.options.front();
		}

		declareSetting(moduleKey, std::move(declaration));
	};

	table["button"] = [this, moduleKey](std::string const &id, std::string const &label, sol::protected_function handler)
	{
		PDSettingDeclaration declaration;
		declaration.id = id;
		declaration.label = label;
		declaration.kind = PDSettingKind::Button;

		if (not declareSetting(moduleKey, std::move(declaration)))
		{
			return;
		}

		sol::reference const traceback = m_lua["debug"]["traceback"];
		m_buttonHandlers[moduleKey + "\n" + id] = sol::protected_function(handler, traceback);
	};

	table["get"] = [this, moduleKey](sol::this_state state, std::string const &id)
	{
		return settingObject(state, moduleKey, std::string(), id);
	};

	return table;
}

void PDLua::forgetButtons(std::string const &moduleKey)
{
	std::string const prefix = moduleKey + "\n";

	for (auto iterator = m_buttonHandlers.begin(); iterator != m_buttonHandlers.end();)
	{
		if (iterator->first.rfind(prefix, 0) == 0)
		{
			iterator = m_buttonHandlers.erase(iterator);
		}
		else
		{
			++iterator;
		}
	}
}

bool PDLua::pressButton(std::string const &moduleKey, std::string const &settingId)
{
	auto const handler = m_buttonHandlers.find(moduleKey + "\n" + settingId);

	if (handler == m_buttonHandlers.end())
	{
		return false;
	}

	return report(moduleKey, handler->second());
}

PDLua::Module *PDLua::module(std::string const &scriptPath)
{
	auto const existing = m_modules.find(scriptPath);

	if (existing != m_modules.end())
	{
		return existing->second.failed ? nullptr : &existing->second;
	}

	std::string const key = PDSettingsStore::moduleKey(m_scriptsRoot, scriptPath);

	Module created;
	created.environment = sol::environment(m_lua, sol::create, m_lua.globals());
	created.environment["settings"] = createSettingsTable(key);

	created.environment["log"] = [this, key](std::string const &message)
	{
		m_diagnostics->write("[" + key + "] " + message);
	};

	forgetButtons(key);
	m_settings->beginModule(key);

	sol::protected_function_result const result = m_lua.script_file(scriptPath, created.environment, sol::script_pass_on_error);

	if (not result.valid())
	{
		sol::error const error = result;
		m_diagnostics->writeOnce(scriptPath, "Script load failed: " + std::string(error.what()));
		m_settings->clearDeclarations(key);
		forgetButtons(key);
		created.failed = true;
		m_modules.emplace(scriptPath, std::move(created));

		return nullptr;
	}

	sol::object const returned = result;

	if (not returned.is<sol::table>())
	{
		m_diagnostics->writeOnce(scriptPath, "Script did not return a table: " + scriptPath);
		m_settings->clearDeclarations(key);
		forgetButtons(key);
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

	std::string const key = PDSettingsStore::moduleKey(m_scriptsRoot, normalized);
	m_settings->clearDeclarations(key);
	forgetButtons(key);

	forgetPackage(normalized);
	m_diagnostics->write("Unloaded script " + normalized);
}

void PDLua::reload()
{
	m_modules.clear();
	m_buttonHandlers.clear();
	m_settings->clearAllDeclarations();
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
