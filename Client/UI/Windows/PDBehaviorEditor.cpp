#include <UI/Windows/PDBehaviorEditor.h>

#include <Core/PDString.h>
#include <Engine/PDDiagnostics.h>
#include <Library/PDPonyPackOverride.h>
#include <UI/PDTheme.h>
#include <UI/PDWidgets.h>

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <fstream>

static nlohmann::json const &behaviorField(nlohmann::json const &behavior, char const *key)
{
	static nlohmann::json const missing;
	auto const found = behavior.find(key);

	return found != behavior.end() ? *found : missing;
}

static std::string fieldString(nlohmann::json const &behavior, char const *key)
{
	nlohmann::json const &value = behaviorField(behavior, key);

	return value.is_string() ? value.get<std::string>() : std::string();
}

static double fieldNumber(nlohmann::json const &behavior, char const *key)
{
	nlohmann::json const &value = behaviorField(behavior, key);

	return value.is_number() ? value.get<double>() : 0.0;
}

static bool fieldBool(nlohmann::json const &behavior, char const *key)
{
	nlohmann::json const &value = behaviorField(behavior, key);

	return value.is_boolean() and value.get<bool>();
}

static int fieldGroup(nlohmann::json const &behavior)
{
	nlohmann::json const &value = behaviorField(behavior, "group");

	return value.is_number_integer() ? value.get<int>() : 0;
}

static double fieldDuration(nlohmann::json const &behavior, char const *key)
{
	nlohmann::json const &duration = behaviorField(behavior, "durationMs");

	if (not duration.is_object())
	{
		return 0.0;
	}

	return fieldNumber(duration, key);
}

PDBehaviorEditor::PDBehaviorEditor(PDDiagnostics &diagnostics)
	: m_diagnostics(diagnostics)
{
}

bool PDBehaviorEditor::open(std::string const &packPath, std::string const &displayName)
{
	std::string const path = ponyPackDocumentPath(packPath);
	std::ifstream stream(path, std::ios::binary);

	if (not stream)
	{
		return false;
	}

	nlohmann::json document = nlohmann::json::parse(stream, nullptr, false);

	if (document.is_discarded() or not document.is_object())
	{
		return false;
	}

	m_document = std::move(document);
	m_packPath = packPath;
	m_displayName = displayName;
	m_hasOverride = ponyPackOverrideExists(packPath);
	m_expanded = -1;
	m_search[0] = '\0';
	m_status.clear();

	rebuildIndex();

	return true;
}

void PDBehaviorEditor::close()
{
	m_packPath.clear();
	m_displayName.clear();
	m_document = nlohmann::json();
	m_modes.clear();
	m_linkTargets.clear();
	m_status.clear();
}

nlohmann::json const &PDBehaviorEditor::behaviors() const
{
	static nlohmann::json const empty = nlohmann::json::array();
	auto const found = m_document.find("behaviors");

	return found != m_document.end() and found->is_array() ? *found : empty;
}

void PDBehaviorEditor::rebuildIndex()
{
	m_modes.clear();
	m_linkTargets.clear();

	nlohmann::json const &pool = behaviors();

	for (nlohmann::json const &behavior : pool)
	{
		std::string const link = fieldString(behavior, "linkedBehavior");

		if (not link.empty())
		{
			m_linkTargets.push_back(link);
		}
	}

	std::sort(m_linkTargets.begin(), m_linkTargets.end());
	m_linkTargets.erase(std::unique(m_linkTargets.begin(), m_linkTargets.end()), m_linkTargets.end());

	auto const declared = m_document.find("behaviorGroups");

	if (declared != m_document.end() and declared->is_array())
	{
		for (nlohmann::json const &entry : *declared)
		{
			if (not entry.is_object())
			{
				continue;
			}

			PDBehaviorMode mode;
			mode.id = entry.value("id", 0);
			mode.name = entry.value("name", std::string());

			if (mode.id != 0 and modeName(mode.id) == nullptr)
			{
				m_modes.push_back(std::move(mode));
			}
		}
	}

	for (nlohmann::json const &behavior : pool)
	{
		int const group = fieldGroup(behavior);

		if (group != 0 and modeName(group) == nullptr)
		{
			PDBehaviorMode mode;
			mode.id = group;
			m_modes.push_back(std::move(mode));
		}
	}

	m_mode = m_modes.empty() ? 0 : m_modes.front().id;
}

char const *PDBehaviorEditor::modeName(int id) const
{
	for (PDBehaviorMode const &mode : m_modes)
	{
		if (mode.id == id)
		{
			return mode.name.c_str();
		}
	}

	return nullptr;
}

bool PDBehaviorEditor::isLinkTarget(std::string const &id) const
{
	return std::find(m_linkTargets.begin(), m_linkTargets.end(), id) != m_linkTargets.end();
}

bool PDBehaviorEditor::eligibleInMode(std::size_t index, int mode) const
{
	nlohmann::json const &behavior = behaviors()[index];

	if (fieldNumber(behavior, "chance") <= 0.0 or fieldBool(behavior, "skip"))
	{
		return false;
	}

	int const group = fieldGroup(behavior);

	return group == 0 or group == mode;
}

double PDBehaviorEditor::eligibleTotal(int mode) const
{
	double total = 0.0;

	for (std::size_t index = 0; index < behaviors().size(); index += 1)
	{
		if (eligibleInMode(index, mode))
		{
			total += fieldNumber(behaviors()[index], "chance");
		}
	}

	return total;
}

void PDBehaviorEditor::drawBehaviorRow(std::size_t index, double total)
{
	nlohmann::json const &behavior = behaviors()[index];
	std::string const id = fieldString(behavior, "id");
	std::string const name = fieldString(behavior, "name");

	PDRowCard const card = beginRowCard(ListRowHeight, m_expanded == static_cast<int>(index));

	const float lineHeight = ImGui::GetTextLineHeight();
	const float firstY = card.position.y + (ListRowHeight - lineHeight * 2.0f - 2.0f) * 0.5f;

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	drawList->AddText(
		ImVec2(card.position.x + 10.0f, firstY),
		ImGui::GetColorU32(PDTheme::White),
		name.empty() ? id.c_str() : name.c_str());

	char subtitle[192];
	std::snprintf(
		subtitle,
		sizeof(subtitle),
		"chance %.6g  -  %.1f-%.1f s  -  %s",
		fieldNumber(behavior, "chance"),
		fieldDuration(behavior, "min") / 1000.0,
		fieldDuration(behavior, "max") / 1000.0,
		fieldString(behavior, "movement").c_str());

	drawList->AddText(
		ImVec2(card.position.x + 10.0f, firstY + lineHeight + 2.0f),
		ImGui::GetColorU32(PDTheme::TextFaint),
		subtitle);

	char chip[64];
	ImVec4 chipColor = PDTheme::White;

	if (eligibleInMode(index, m_mode))
	{
		double const share = total > 0.0 ? fieldNumber(behavior, "chance") / total * 100.0 : 0.0;
		std::snprintf(chip, sizeof(chip), "%.1f %%", share);
	}
	else
	{
		int const group = fieldGroup(behavior);

		if (group != 0 and group != m_mode)
		{
			std::snprintf(chip, sizeof(chip), "other mode");
			chipColor = PDTheme::TextFaint;
		}
		else if (isLinkTarget(id))
		{
			std::snprintf(chip, sizeof(chip), "link only");
			chipColor = PDTheme::TextDim;
		}
		else
		{
			std::snprintf(chip, sizeof(chip), "unreachable");
			chipColor = PDTheme::Stop;
		}
	}

	const float chipWidth = ImGui::CalcTextSize(chip).x;
	drawList->AddText(
		ImVec2(card.rectMax.x - 10.0f - chipWidth, firstY),
		ImGui::GetColorU32(chipColor),
		chip);

	int const group = fieldGroup(behavior);

	if (group != 0)
	{
		char const *const label = modeName(group);
		char badge[64];
		std::snprintf(badge, sizeof(badge), "%s", label != nullptr and *label != '\0' ? label : "grouped");

		const float badgeWidth = ImGui::CalcTextSize(badge).x;
		drawList->AddText(
			ImVec2(card.rectMax.x - 10.0f - badgeWidth, firstY + lineHeight + 2.0f),
			ImGui::GetColorU32(PDTheme::AccentText),
			badge);
	}

	if (card.clicked)
	{
		m_expanded = m_expanded == static_cast<int>(index) ? -1 : static_cast<int>(index);
	}

	endRowCard(card, ListRowHeight, 4.0f);
}

bool PDBehaviorEditor::draw()
{
	bool back = false;

	beginToolbar();

	if (ImGui::Button("Back"))
	{
		back = true;
	}

	if (m_modes.size() > 1)
	{
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ModePickerWidth);

		std::vector<std::string> labels;
		std::vector<const char *> items;
		int current = 0;

		for (std::size_t index = 0; index < m_modes.size(); index += 1)
		{
			char label[96];

			if (m_modes[index].name.empty())
			{
				std::snprintf(label, sizeof(label), "Mode %d", m_modes[index].id);
			}
			else
			{
				std::snprintf(label, sizeof(label), "%s", m_modes[index].name.c_str());
			}

			labels.push_back(label);

			if (m_modes[index].id == m_mode)
			{
				current = static_cast<int>(index);
			}
		}

		for (std::string const &label : labels)
		{
			items.push_back(label.c_str());
		}

		if (ImGui::Combo("##mode", &current, items.data(), static_cast<int>(items.size())))
		{
			m_mode = m_modes[static_cast<std::size_t>(current)].id;
		}
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(180.0f);
	ImGui::InputTextWithHint("##behaviorSearch", "Search behaviors", m_search, sizeof(m_search));

	char summary[160];
	std::snprintf(
		summary,
		sizeof(summary),
		"%s  -  %zu behaviors%s",
		m_displayName.c_str(),
		behaviors().size(),
		m_hasOverride ? "  -  overridden" : "");
	toolbarSummary(summary);

	endToolbar();

	if (back)
	{
		return false;
	}

	std::string const filter = toLower(m_search);
	double const total = eligibleTotal(m_mode);

	ImGui::BeginChild("behaviorList");

	for (std::size_t index = 0; index < behaviors().size(); index += 1)
	{
		nlohmann::json const &behavior = behaviors()[index];

		if (not filter.empty())
		{
			std::string const haystack = toLower(fieldString(behavior, "id") + " " + fieldString(behavior, "name"));

			if (haystack.find(filter) == std::string::npos)
			{
				continue;
			}
		}

		ImGui::PushID(static_cast<int>(index));
		drawBehaviorRow(index, total);
		ImGui::PopID();
	}

	ImGui::EndChild();

	return true;
}
