#pragma once

#include <Library/PDPonyPackData.h>
#include <Library/PDSettingsStore.h>

#include <sol/sol.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class PDDiagnostics;

class PDLua
{
public:
	using PlayHandler = std::function<bool(std::uint32_t, std::string const &, bool, bool)>;
	using FacingHandler = std::function<void(std::uint32_t, bool)>;

	void initialize(std::string const &scriptsRoot, PDDiagnostics &diagnostics, PDSettingsStore &settings);

	void setPlayHandler(PlayHandler handler);
	void setFacingHandler(FacingHandler handler);

	void beginFrame(float boundsWidth, float boundsHeight);

	sol::table createSelf(std::uint32_t entityId, std::string const &scriptPath, std::string const &packId);
	sol::table packTable(PDPonyPackData const &pack);

	bool pressButton(std::string const &moduleKey, std::string const &settingId);

	bool callSpawn(std::string const &scriptPath, sol::table &self);
	bool callTick(std::string const &scriptPath, sol::table &self, float deltaSeconds);

	bool loadScript(std::string const &scriptPath);
	void unloadScript(std::string const &scriptPath);

	void reload();

	std::vector<std::string> loadedScripts() const;

private:
	struct Module
	{
		sol::environment environment;
		sol::protected_function spawn;
		sol::protected_function tick;
		bool failed = false;
	};

	Module *module(std::string const &scriptPath);
	bool report(std::string const &scriptPath, sol::protected_function_result const &result);
	std::string modulePath(std::string const &name) const;
	void forgetPackage(std::string const &normalizedPath);

	sol::table createSettingsTable(std::string const &moduleKey);
	bool declareSetting(std::string const &moduleKey, PDSettingDeclaration declaration);
	PDSettingValue settingValue(std::string const &moduleKey, std::string const &packId, std::string const &id) const;
	sol::object settingObject(sol::this_state state, std::string const &moduleKey, std::string const &packId, std::string const &id) const;
	void forgetButtons(std::string const &moduleKey);

	sol::state m_lua;
	PDDiagnostics *m_diagnostics = nullptr;
	std::string m_scriptsRoot;

	sol::table m_metatable;
	sol::table m_frame;

	PlayHandler m_playHandler;
	FacingHandler m_facingHandler;

	std::unordered_map<std::string, Module> m_modules;
	std::unordered_map<std::string, sol::table> m_packTables;

	PDSettingsStore *m_settings = nullptr;
	PDSettingsSnapshot m_snapshot;
	std::unordered_map<std::string, sol::protected_function> m_buttonHandlers;
};
