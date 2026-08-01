#pragma once

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>
#include <vector>

class PDDiagnostics;

struct PDBehaviorMode
{
	int id = 0;
	std::string name;
};

class PDBehaviorEditor
{
public:
	explicit PDBehaviorEditor(PDDiagnostics &diagnostics);

	bool open(std::string const &packPath, std::string const &displayName);
	void close();

	bool isOpen() const
	{
		return not m_packPath.empty();
	}

	bool draw();

private:
	static constexpr float ListRowHeight = 46.0f;
	static constexpr float ModePickerWidth = 150.0f;

	void rebuildIndex();
	void drawBehaviorRow(std::size_t index, double total);

	nlohmann::json const &behaviors() const;
	bool eligibleInMode(std::size_t index, int mode) const;
	double eligibleTotal(int mode) const;
	char const *modeName(int id) const;
	bool isLinkTarget(std::string const &id) const;

	PDDiagnostics &m_diagnostics;

	std::string m_packPath;
	std::string m_displayName;
	std::string m_status;

	nlohmann::json m_document;

	std::vector<PDBehaviorMode> m_modes;
	std::vector<std::string> m_linkTargets;

	int m_mode = 0;
	int m_expanded = -1;
	bool m_hasOverride = false;
	char m_search[96] = "";
};
