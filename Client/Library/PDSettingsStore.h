#pragma once

#include <Library/PDSettings.h>

#include <atomic>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct PDSettingsModuleSnapshot
{
	std::unordered_map<std::string, PDSettingValue> values;
	std::unordered_map<std::string, std::unordered_map<std::string, PDSettingValue>> packs;
};

struct PDSettingsSnapshot
{
	std::uint64_t version = 0;
	std::unordered_map<std::string, PDSettingsModuleSnapshot> modules;
};

class PDSettingsStore
{
public:
	PDSettingsStore();

	static std::string moduleKey(std::string const &scriptsRoot, std::string const &fullPath);

	bool load();
	bool save() const;

	bool exportTo(std::string const &path) const;
	bool importFrom(std::string const &path);

	std::string const &path() const
	{
		return m_path;
	}

	void beginModule(std::string const &key);
	bool declare(std::string const &key, PDSettingDeclaration declaration, std::string &outWarning);
	void clearDeclarations(std::string const &key);
	void clearAllDeclarations();

	bool refresh(PDSettingsSnapshot &snapshot) const;

	std::vector<PDSettingDeclaration> declarations(std::string const &key) const;
	PDSettingValue value(std::string const &key, std::string const &packId, std::string const &id) const;
	bool hasOverride(std::string const &key, std::string const &packId, std::string const &id) const;
	bool hasStoredValue(std::string const &key, std::string const &id) const;
	std::vector<std::string> packsWithOverrides(std::string const &key) const;

	void setValue(std::string const &key, std::string const &packId, std::string const &id, PDSettingValue value);
	void clearValue(std::string const &key, std::string const &packId, std::string const &id);

	bool appFlag(std::string const &id, bool fallback) const;
	void setAppFlag(std::string const &id, bool value);

private:
	static constexpr std::size_t MaxDeclarations = 64;

	using PDSettingValues = std::unordered_map<std::string, PDSettingValue>;

	struct Module
	{
		std::vector<PDSettingDeclaration> declarations;
		PDSettingValues values;
		std::unordered_map<std::string, PDSettingValues> packs;
	};

	static PDSettingValueType expectedType(PDSettingKind kind);
	static void reconcile(PDSettingDeclaration const &declaration, PDSettingValues &values);

	bool readFrom(std::string const &path, bool replace);
	bool writeTo(std::string const &path) const;

	Module const *findModule(std::string const &key) const;

	mutable std::shared_mutex m_mutex;
	std::atomic<std::uint64_t> m_version = 1;
	std::string m_path;
	std::unordered_map<std::string, Module> m_modules;
	PDSettingValues m_app;
};
