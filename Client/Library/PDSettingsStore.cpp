#include <Library/PDSettingsStore.h>

#include <windows.h>

#include <shlobj.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <utility>

static std::string resolveSettingsPath()
{
	PWSTR folder = nullptr;

	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &folder)))
	{
		return "settings.json";
	}

	std::filesystem::path path(folder);
	CoTaskMemFree(folder);

	path /= "Pony Dock";
	path /= "settings.json";

	return path.string();
}

static nlohmann::json settingToJson(PDSettingValue const &value)
{
	switch (value.type)
	{
		case PDSettingValueType::Boolean:
		{
			return value.boolean;
		}

		case PDSettingValueType::Number:
		{
			return value.number;
		}

		case PDSettingValueType::Text:
		{
			return value.text;
		}

		default:
		{
			return nullptr;
		}
	}
}

static PDSettingValue settingFromJson(nlohmann::json const &node)
{
	PDSettingValue value;

	if (node.is_boolean())
	{
		value.type = PDSettingValueType::Boolean;
		value.boolean = node.get<bool>();
	}
	else if (node.is_number())
	{
		value.type = PDSettingValueType::Number;
		value.number = node.get<float>();
	}
	else if (node.is_string())
	{
		value.type = PDSettingValueType::Text;
		value.text = node.get<std::string>();
	}

	return value;
}

static void readValues(nlohmann::json const &node, std::unordered_map<std::string, PDSettingValue> &values)
{
	if (not node.is_object())
	{
		return;
	}

	for (auto const &entry : node.items())
	{
		PDSettingValue parsed = settingFromJson(entry.value());

		if (parsed.type != PDSettingValueType::None)
		{
			values[entry.key()] = std::move(parsed);
		}
	}
}

static nlohmann::json writeValues(std::unordered_map<std::string, PDSettingValue> const &values)
{
	nlohmann::json node = nlohmann::json::object();

	for (auto const &entry : values)
	{
		nlohmann::json const value = settingToJson(entry.second);

		if (not value.is_null())
		{
			node[entry.first] = value;
		}
	}

	return node;
}

PDSettingsStore::PDSettingsStore()
	: m_path(resolveSettingsPath())
{
}

std::string PDSettingsStore::moduleKey(std::string const &scriptsRoot, std::string const &fullPath)
{
	std::filesystem::path const root = std::filesystem::path(scriptsRoot).lexically_normal();
	std::filesystem::path const full = std::filesystem::path(fullPath).lexically_normal();
	std::filesystem::path const relative = full.lexically_relative(root);

	if (relative.empty())
	{
		return full.filename().generic_string();
	}

	return relative.generic_string();
}

PDSettingValueType PDSettingsStore::expectedType(PDSettingKind kind)
{
	switch (kind)
	{
		case PDSettingKind::Checkbox:
		{
			return PDSettingValueType::Boolean;
		}

		case PDSettingKind::Slider:
		{
			return PDSettingValueType::Number;
		}

		case PDSettingKind::Text:
		{
			return PDSettingValueType::Text;
		}

		case PDSettingKind::Dropdown:
		{
			return PDSettingValueType::Text;
		}

		default:
		{
			return PDSettingValueType::None;
		}
	}
}

void PDSettingsStore::reconcile(PDSettingDeclaration const &declaration, PDSettingValues &values)
{
	auto const existing = values.find(declaration.id);

	if (existing == values.end())
	{
		return;
	}

	if (existing->second.type != expectedType(declaration.kind))
	{
		values.erase(existing);

		return;
	}

	switch (declaration.kind)
	{
		case PDSettingKind::Slider:
		{
			existing->second.number = std::clamp(existing->second.number, declaration.minimum, declaration.maximum);

			break;
		}

		case PDSettingKind::Dropdown:
		{
			if (std::find(declaration.options.begin(), declaration.options.end(), existing->second.text) == declaration.options.end())
			{
				values.erase(existing);
			}

			break;
		}

		default:
		{
			break;
		}
	}
}

PDSettingsStore::Module const *PDSettingsStore::findModule(std::string const &key) const
{
	auto const existing = m_modules.find(key);

	if (existing == m_modules.end())
	{
		return nullptr;
	}

	return &existing->second;
}

void PDSettingsStore::beginModule(std::string const &key)
{
	std::unique_lock<std::shared_mutex> const lock(m_mutex);
	m_modules[key].declarations.clear();
	m_version.fetch_add(1, std::memory_order_release);
}

bool PDSettingsStore::declare(std::string const &key, PDSettingDeclaration declaration, std::string &outWarning)
{
	if (declaration.id.empty())
	{
		outWarning = "setting declared without an id";

		return false;
	}

	if (declaration.kind == PDSettingKind::Dropdown)
	{
		if (declaration.options.empty())
		{
			outWarning = "dropdown '" + declaration.id + "' declared with no options";

			return false;
		}

		if (std::find(declaration.options.begin(), declaration.options.end(), declaration.defaultValue.text) == declaration.options.end())
		{
			outWarning = "dropdown '" + declaration.id + "' default is not one of its options";
			declaration.defaultValue.text = declaration.options.front();
		}
	}

	if (declaration.kind == PDSettingKind::Slider and declaration.maximum < declaration.minimum)
	{
		outWarning = "slider '" + declaration.id + "' has a maximum below its minimum";
		std::swap(declaration.minimum, declaration.maximum);
	}

	std::unique_lock<std::shared_mutex> const lock(m_mutex);
	Module &module = m_modules[key];

	if (declaration.kind == PDSettingKind::Button)
	{
		module.values.erase(declaration.id);

		for (std::pair<std::string const, PDSettingValues> &pack : module.packs)
		{
			pack.second.erase(declaration.id);
		}
	}
	else
	{
		reconcile(declaration, module.values);

		for (std::pair<std::string const, PDSettingValues> &pack : module.packs)
		{
			reconcile(declaration, pack.second);
		}
	}

	for (PDSettingDeclaration &current : module.declarations)
	{
		if (current.id == declaration.id)
		{
			current = std::move(declaration);
			m_version.fetch_add(1, std::memory_order_release);

			return true;
		}
	}

	if (module.declarations.size() >= MaxDeclarations)
	{
		outWarning = "module '" + key + "' declared more than " + std::to_string(MaxDeclarations) + " settings";

		return false;
	}

	module.declarations.push_back(std::move(declaration));
	m_version.fetch_add(1, std::memory_order_release);

	return true;
}

void PDSettingsStore::clearDeclarations(std::string const &key)
{
	std::unique_lock<std::shared_mutex> const lock(m_mutex);

	auto const existing = m_modules.find(key);

	if (existing != m_modules.end())
	{
		existing->second.declarations.clear();
	}

	m_version.fetch_add(1, std::memory_order_release);
}

void PDSettingsStore::clearAllDeclarations()
{
	std::unique_lock<std::shared_mutex> const lock(m_mutex);

	for (std::pair<std::string const, Module> &entry : m_modules)
	{
		entry.second.declarations.clear();
	}

	m_version.fetch_add(1, std::memory_order_release);
}

bool PDSettingsStore::refresh(PDSettingsSnapshot &snapshot) const
{
	std::uint64_t const version = m_version.load(std::memory_order_acquire);

	if (snapshot.version == version)
	{
		return false;
	}

	std::shared_lock<std::shared_mutex> const lock(m_mutex);
	snapshot.modules.clear();

	for (std::pair<std::string const, Module> const &entry : m_modules)
	{
		PDSettingsModuleSnapshot &target = snapshot.modules[entry.first];
		target.values = entry.second.values;
		target.packs = entry.second.packs;

		for (PDSettingDeclaration const &declaration : entry.second.declarations)
		{
			if (declaration.kind == PDSettingKind::Button)
			{
				continue;
			}

			if (target.values.find(declaration.id) == target.values.end())
			{
				target.values[declaration.id] = declaration.defaultValue;
			}
		}
	}

	snapshot.version = version;

	return true;
}

std::vector<PDSettingDeclaration> PDSettingsStore::declarations(std::string const &key) const
{
	std::shared_lock<std::shared_mutex> const lock(m_mutex);
	Module const *const module = findModule(key);

	if (module == nullptr)
	{
		return {};
	}

	return module->declarations;
}

PDSettingValue PDSettingsStore::value(std::string const &key, std::string const &packId, std::string const &id) const
{
	std::shared_lock<std::shared_mutex> const lock(m_mutex);
	Module const *const module = findModule(key);

	if (module == nullptr)
	{
		return {};
	}

	if (not packId.empty())
	{
		auto const pack = module->packs.find(packId);

		if (pack != module->packs.end())
		{
			auto const override = pack->second.find(id);

			if (override != pack->second.end())
			{
				return override->second;
			}
		}
	}

	auto const stored = module->values.find(id);

	if (stored != module->values.end())
	{
		return stored->second;
	}

	for (PDSettingDeclaration const &declaration : module->declarations)
	{
		if (declaration.id == id)
		{
			return declaration.defaultValue;
		}
	}

	return {};
}

bool PDSettingsStore::hasOverride(std::string const &key, std::string const &packId, std::string const &id) const
{
	if (packId.empty())
	{
		return false;
	}

	std::shared_lock<std::shared_mutex> const lock(m_mutex);
	Module const *const module = findModule(key);

	if (module == nullptr)
	{
		return false;
	}

	auto const pack = module->packs.find(packId);

	if (pack == module->packs.end())
	{
		return false;
	}

	return pack->second.find(id) != pack->second.end();
}

bool PDSettingsStore::hasStoredValue(std::string const &key, std::string const &id) const
{
	std::shared_lock<std::shared_mutex> const lock(m_mutex);
	Module const *const module = findModule(key);

	if (module == nullptr)
	{
		return false;
	}

	return module->values.find(id) != module->values.end();
}

std::vector<std::string> PDSettingsStore::packsWithOverrides(std::string const &key) const
{
	std::shared_lock<std::shared_mutex> const lock(m_mutex);
	Module const *const module = findModule(key);

	if (module == nullptr)
	{
		return {};
	}

	std::vector<std::string> result;

	for (std::pair<std::string const, PDSettingValues> const &pack : module->packs)
	{
		if (not pack.second.empty())
		{
			result.push_back(pack.first);
		}
	}

	std::sort(result.begin(), result.end());

	return result;
}

void PDSettingsStore::setValue(std::string const &key, std::string const &packId, std::string const &id, PDSettingValue value)
{
	std::unique_lock<std::shared_mutex> const lock(m_mutex);
	Module &module = m_modules[key];

	if (packId.empty())
	{
		module.values[id] = std::move(value);
	}
	else
	{
		module.packs[packId][id] = std::move(value);
	}

	m_version.fetch_add(1, std::memory_order_release);
}

void PDSettingsStore::clearValue(std::string const &key, std::string const &packId, std::string const &id)
{
	std::unique_lock<std::shared_mutex> const lock(m_mutex);

	auto const existing = m_modules.find(key);

	if (existing == m_modules.end())
	{
		return;
	}

	if (packId.empty())
	{
		existing->second.values.erase(id);
	}
	else
	{
		auto const pack = existing->second.packs.find(packId);

		if (pack != existing->second.packs.end())
		{
			pack->second.erase(id);

			if (pack->second.empty())
			{
				existing->second.packs.erase(pack);
			}
		}
	}

	m_version.fetch_add(1, std::memory_order_release);
}

bool PDSettingsStore::load()
{
	std::ifstream stream(m_path, std::ios::binary);

	if (not stream)
	{
		return false;
	}

	nlohmann::json const document = nlohmann::json::parse(stream, nullptr, false);

	if (document.is_discarded() or not document.is_object())
	{
		return false;
	}

	auto const modules = document.find("modules");

	if (modules == document.end() or not modules->is_object())
	{
		return false;
	}

	std::unique_lock<std::shared_mutex> const lock(m_mutex);

	for (auto const &entry : modules->items())
	{
		if (not entry.value().is_object())
		{
			continue;
		}

		Module &module = m_modules[entry.key()];
		auto const values = entry.value().find("settings");

		if (values != entry.value().end())
		{
			readValues(*values, module.values);
		}

		auto const packs = entry.value().find("packs");

		if (packs == entry.value().end() or not packs->is_object())
		{
			continue;
		}

		for (auto const &pack : packs->items())
		{
			readValues(pack.value(), module.packs[pack.key()]);
		}
	}

	m_version.fetch_add(1, std::memory_order_release);

	return true;
}

bool PDSettingsStore::save() const
{
	nlohmann::json document;

	{
		std::shared_lock<std::shared_mutex> const lock(m_mutex);
		nlohmann::json modules = nlohmann::json::object();

		for (std::pair<std::string const, Module> const &entry : m_modules)
		{
			nlohmann::json values = writeValues(entry.second.values);
			nlohmann::json packs = nlohmann::json::object();

			for (std::pair<std::string const, PDSettingValues> const &pack : entry.second.packs)
			{
				nlohmann::json packValues = writeValues(pack.second);

				if (not packValues.empty())
				{
					packs[pack.first] = std::move(packValues);
				}
			}

			if (values.empty() and packs.empty())
			{
				continue;
			}

			nlohmann::json module = nlohmann::json::object();
			module["settings"] = std::move(values);

			if (not packs.empty())
			{
				module["packs"] = std::move(packs);
			}

			modules[entry.first] = std::move(module);
		}

		document["version"] = 1;
		document["modules"] = std::move(modules);
	}

	std::filesystem::path const target(m_path);
	std::error_code error;
	std::filesystem::create_directories(target.parent_path(), error);

	std::filesystem::path temporary = target;
	temporary += ".tmp";

	{
		std::ofstream stream(temporary, std::ios::binary);

		if (not stream)
		{
			return false;
		}

		stream << document.dump(2);
	}

	std::filesystem::rename(temporary, target, error);

	return not error;
}
