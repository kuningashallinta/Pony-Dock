#pragma once

#include <nlohmann/json.hpp>

#include <imgui.h>

#include <cstddef>
#include <string>
#include <vector>

class PDDiagnostics;
class PDMainApplication;

struct PDBehaviorMode
{
	int id = 0;
	std::string name;
};

class PDBehaviorEditor
{
public:
	PDBehaviorEditor(PDMainApplication &app, PDDiagnostics &diagnostics);

	bool open(std::string const &packPath, std::string const &displayName);
	void close();

	bool isOpen() const
	{
		return not m_packPath.empty();
	}

	bool draw();
	void drawDetail();

private:
	static constexpr float ListRowHeight = 46.0f;
	static constexpr float LabelWidth = 150.0f;
	static constexpr float ModePickerWidth = 150.0f;
	static constexpr std::size_t NoBaseIndex = static_cast<std::size_t>(-1);

	void rebuildIndex();
	void drawBehaviorRow(std::size_t index, double total);
	void drawFields(std::size_t index);
	void drawResetModal();

	void drawFieldLabel(char const *label);
	void drawRevertMarker(std::size_t index, char const *key);

	void drawNameField(std::size_t index);
	void drawChanceField(std::size_t index);
	void drawSpeedField(std::size_t index);
	void drawDurationField(std::size_t index);
	void drawGroupField(std::size_t index);
	void drawFlagField(std::size_t index, char const *key, char const *label);
	void drawChoiceField(
		std::size_t index,
		char const *key,
		char const *label,
		std::vector<std::string> const &options,
		bool allowNone);

	nlohmann::json &behavior(std::size_t index);
	nlohmann::json const &behaviors() const;
	nlohmann::json const *baseValue(std::size_t index, char const *key) const;

	bool eligibleInMode(std::size_t index, int mode) const;
	double eligibleTotal(int mode) const;
	char const *modeName(int id) const;
	bool isLinkTarget(std::string const &id) const;

	void commit();
	void revertField(std::size_t index, char const *key);
	void resetToPack();

	PDMainApplication &m_app;
	PDDiagnostics &m_diagnostics;

	std::string m_packPath;
	std::string m_displayName;
	std::string m_status;

	nlohmann::json m_document;
	nlohmann::json m_base;

	std::vector<PDBehaviorMode> m_modes;
	std::vector<std::string> m_linkTargets;
	std::vector<std::string> m_behaviorIds;
	std::vector<std::string> m_animations;
	std::vector<std::string> m_movements;
	std::vector<std::size_t> m_baseByIndex;

	int m_mode = 0;
	int m_expanded = -1;
	bool m_hasOverride = false;
	bool m_resetOpen = false;
	char m_search[96] = "";
};
