#include <UI/Windows/PDBehaviorEditor.h>

#include <App/PDMainApplication.h>
#include <Core/PDString.h>
#include <Engine/PDDiagnostics.h>
#include <Library/PDPonyPackOverride.h>
#include <UI/PDTheme.h>
#include <UI/PDWidgets.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
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

static bool readDocument(std::string const &path, nlohmann::json &outDocument)
{
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

	outDocument = std::move(document);

	return true;
}

PDBehaviorEditor::PDBehaviorEditor(PDMainApplication &app, PDDiagnostics &diagnostics)
	: m_app(app),
	  m_diagnostics(diagnostics)
{
	m_movements = {
		"None",
		"MouseOver",
		"Sleep",
		"Dragged",
		"Horizontal_Only",
		"Vertical_Only",
		"Diagonal_Only",
		"Horizontal_Vertical",
		"Diagonal_Horizontal",
		"Diagonal_Vertical",
		"All"};
}

bool PDBehaviorEditor::open(std::string const &packPath, std::string const &displayName)
{
	nlohmann::json document;

	if (not readDocument(ponyPackDocumentPath(packPath), document))
	{
		return false;
	}

	m_document = std::move(document);
	m_base = nlohmann::json();
	readDocument((std::filesystem::path(packPath) / "pony.json").string(), m_base);

	m_packPath = packPath;
	m_displayName = displayName;
	m_hasOverride = ponyPackOverrideExists(packPath);
	m_expanded = -1;
	m_resetOpen = false;
	m_search[0] = '\0';
	m_status.clear();
	m_mode = 0;

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
	int const previousMode = m_mode;

	m_modes.clear();
	m_linkTargets.clear();
	m_behaviorIds.clear();
	m_animations.clear();
	m_baseByIndex.clear();

	nlohmann::json const &pool = behaviors();

	auto const animations = m_document.find("animations");

	if (animations != m_document.end() and animations->is_object())
	{
		for (auto const &entry : animations->items())
		{
			if (not entry.value().is_object())
			{
				continue;
			}

			if (fieldString(entry.value(), "left").empty() and fieldString(entry.value(), "right").empty())
			{
				continue;
			}

			m_animations.push_back(entry.key());
		}
	}

	std::sort(m_animations.begin(), m_animations.end());

	for (nlohmann::json const &entry : pool)
	{
		std::string const link = fieldString(entry, "linkedBehavior");

		if (not link.empty())
		{
			m_linkTargets.push_back(link);
		}

		m_behaviorIds.push_back(fieldString(entry, "id"));
	}

	auto const basePool = m_base.find("behaviors");

	for (std::string const &id : m_behaviorIds)
	{
		std::size_t match = NoBaseIndex;

		if (basePool != m_base.end() and basePool->is_array())
		{
			for (std::size_t index = 0; index < basePool->size(); index += 1)
			{
				if (fieldString((*basePool)[index], "id") == id)
				{
					match = index;

					break;
				}
			}
		}

		m_baseByIndex.push_back(match);
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

	for (PDBehaviorMode const &mode : m_modes)
	{
		if (mode.id == previousMode)
		{
			m_mode = previousMode;

			break;
		}
	}
}

nlohmann::json &PDBehaviorEditor::behavior(std::size_t index)
{
	return m_document["behaviors"][index];
}

nlohmann::json const *PDBehaviorEditor::baseValue(std::size_t index, char const *key) const
{
	if (index >= m_baseByIndex.size() or m_baseByIndex[index] == NoBaseIndex)
	{
		return nullptr;
	}

	auto const basePool = m_base.find("behaviors");

	if (basePool == m_base.end() or not basePool->is_array())
	{
		return nullptr;
	}

	nlohmann::json const &entry = (*basePool)[m_baseByIndex[index]];
	auto const found = entry.find(key);

	return found != entry.end() ? &(*found) : nullptr;
}

void PDBehaviorEditor::commit()
{
	std::string error;

	if (not writePonyPackOverride(m_packPath, m_document.dump(2), error))
	{
		m_status = error;
		m_diagnostics.write("Behavior editor: " + error);

		return;
	}

	m_hasOverride = true;
	m_status.clear();
	rebuildIndex();
	m_app.reloadPack(m_packPath);
}

void PDBehaviorEditor::revertField(std::size_t index, char const *key)
{
	nlohmann::json const *const base = baseValue(index, key);

	if (base == nullptr)
	{
		behavior(index).erase(key);
	}
	else
	{
		behavior(index)[key] = *base;
	}

	commit();
}

void PDBehaviorEditor::resetToPack()
{
	std::string error;

	if (not removePonyPackOverride(m_packPath, error))
	{
		m_status = error;
		m_diagnostics.write("Behavior editor: " + error);

		return;
	}

	m_hasOverride = false;
	m_status.clear();

	if (not m_base.is_null())
	{
		m_document = m_base;
	}

	rebuildIndex();
	m_app.reloadPack(m_packPath);
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

void PDBehaviorEditor::drawFieldLabel(char const *label)
{
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::SameLine(LabelWidth);
	ImGui::SetNextItemWidth(-(ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x));
}

void PDBehaviorEditor::drawRevertMarker(std::size_t index, char const *key)
{
	nlohmann::json const *const base = baseValue(index, key);
	nlohmann::json const &current = behaviorField(behaviors()[index], key);
	const bool present = not current.is_null();
	const bool modified = base == nullptr ? present : (not present or *base != current);
	const float size = ImGui::GetFrameHeight();

	ImGui::SameLine();

	if (not modified)
	{
		ImGui::Dummy(ImVec2(size, size));

		return;
	}

	if (ImGui::Button("x", ImVec2(size, size)))
	{
		revertField(index, key);
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset to the pack value");
	}
}

void PDBehaviorEditor::drawNameField(std::size_t index)
{
	ImGui::PushID("name");
	drawFieldLabel("Name");

	std::string value = fieldString(behaviors()[index], "name");

	if (ImGui::InputText("##v", &value))
	{
		behavior(index)["name"] = value;
	}

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		commit();
	}

	drawRevertMarker(index, "name");
	ImGui::PopID();
}

void PDBehaviorEditor::drawChanceField(std::size_t index)
{
	ImGui::PushID("chance");
	drawFieldLabel("Chance");

	double value = fieldNumber(behaviors()[index], "chance");

	if (ImGui::InputDouble("##v", &value, 0.0, 0.0, "%.6g"))
	{
		behavior(index)["chance"] = value < 0.0 ? 0.0 : value;
	}

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		commit();
	}

	drawRevertMarker(index, "chance");
	ImGui::PopID();
}

void PDBehaviorEditor::drawSpeedField(std::size_t index)
{
	ImGui::PushID("speed");
	drawFieldLabel("Speed (px/s)");

	double value = fieldNumber(behaviors()[index], "speedPxPerSec");

	if (ImGui::InputDouble("##v", &value, 0.0, 0.0, "%.6g"))
	{
		behavior(index)["speedPxPerSec"] = value < 0.0 ? 0.0 : value;
	}

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		commit();
	}

	drawRevertMarker(index, "speedPxPerSec");
	ImGui::PopID();
}

void PDBehaviorEditor::drawDurationField(std::size_t index)
{
	ImGui::PushID("durationMs");
	drawFieldLabel("Duration (ms)");

	int values[2] = {
		static_cast<int>(fieldDuration(behaviors()[index], "min")),
		static_cast<int>(fieldDuration(behaviors()[index], "max"))};

	if (ImGui::InputInt2("##v", values))
	{
		values[0] = values[0] < 0 ? 0 : values[0];
		values[1] = values[1] < values[0] ? values[0] : values[1];

		behavior(index)["durationMs"]["min"] = values[0];
		behavior(index)["durationMs"]["max"] = values[1];
	}

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		commit();
	}

	drawRevertMarker(index, "durationMs");
	ImGui::PopID();
}

void PDBehaviorEditor::drawGroupField(std::size_t index)
{
	ImGui::PushID("group");
	drawFieldLabel("Mode");

	std::vector<std::string> labels;
	labels.push_back("0  -  any mode");

	int current = 0;
	int const group = fieldGroup(behaviors()[index]);

	for (std::size_t entry = 0; entry < m_modes.size(); entry += 1)
	{
		char label[96];
		std::snprintf(
			label,
			sizeof(label),
			"%d  -  %s",
			m_modes[entry].id,
			m_modes[entry].name.empty() ? "unnamed" : m_modes[entry].name.c_str());
		labels.push_back(label);

		if (m_modes[entry].id == group)
		{
			current = static_cast<int>(entry) + 1;
		}
	}

	std::vector<const char *> items;

	for (std::string const &label : labels)
	{
		items.push_back(label.c_str());
	}

	if (ImGui::Combo("##v", &current, items.data(), static_cast<int>(items.size())))
	{
		behavior(index)["group"] = current == 0 ? 0 : m_modes[static_cast<std::size_t>(current - 1)].id;
		commit();
	}

	drawRevertMarker(index, "group");
	ImGui::PopID();
}

void PDBehaviorEditor::drawFlagField(std::size_t index, char const *key, char const *label)
{
	ImGui::PushID(key);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::SameLine(LabelWidth);

	bool value = fieldBool(behaviors()[index], key);

	if (ImGui::Checkbox("##v", &value))
	{
		behavior(index)[key] = value;
		commit();
	}

	drawRevertMarker(index, key);
	ImGui::PopID();
}

void PDBehaviorEditor::drawChoiceField(
	std::size_t index,
	char const *key,
	char const *label,
	std::vector<std::string> const &options,
	bool allowNone)
{
	ImGui::PushID(key);
	drawFieldLabel(label);

	std::string const value = fieldString(behaviors()[index], key);
	std::vector<std::string> labels;

	if (allowNone)
	{
		labels.push_back("(none)");
	}

	int current = 0;

	for (std::string const &option : options)
	{
		if (option == value)
		{
			current = static_cast<int>(labels.size());
		}

		labels.push_back(option);
	}

	if (not value.empty() and std::find(options.begin(), options.end(), value) == options.end())
	{
		current = static_cast<int>(labels.size());
		labels.push_back(value);
	}

	std::vector<const char *> items;

	for (std::string const &entry : labels)
	{
		items.push_back(entry.c_str());
	}

	if (ImGui::Combo("##v", &current, items.data(), static_cast<int>(items.size())))
	{
		if (allowNone and current == 0)
		{
			behavior(index).erase(key);
		}
		else
		{
			behavior(index)[key] = labels[static_cast<std::size_t>(current)];
		}

		commit();
	}

	drawRevertMarker(index, key);
	ImGui::PopID();
}

void PDBehaviorEditor::drawFields(std::size_t index)
{
	drawNameField(index);
	drawChoiceField(index, "animation", "Animation", m_animations, false);
	drawChanceField(index);
	drawDurationField(index);
	drawChoiceField(index, "movement", "Movement", m_movements, false);
	drawSpeedField(index);
	drawChoiceField(index, "linkedBehavior", "Play next", m_behaviorIds, true);
	drawGroupField(index);

	ImGui::Dummy(ImVec2(0.0f, 6.0f));

	drawFlagField(index, "skip", "Never pick at random");
	drawFlagField(index, "special", "Extended behavior");
	drawFlagField(index, "preventAnimationLoop", "Play animation once");
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

void PDBehaviorEditor::drawDetail()
{
	if (m_expanded < 0 or m_expanded >= static_cast<int>(behaviors().size()))
	{
		return;
	}

	std::size_t const index = static_cast<std::size_t>(m_expanded);
	std::string const id = fieldString(behaviors()[index], "id");
	std::string const name = fieldString(behaviors()[index], "name");

	char title[256];
	std::snprintf(
		title,
		sizeof(title),
		"%s  -  %s###behaviorDetail",
		m_displayName.c_str(),
		name.empty() ? id.c_str() : name.c_str());

	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(480.0f, 0.0f), ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, 0.0f), ImVec2(900.0f, 4000.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));

	if (ImGui::Begin(title, &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextFaint);
		ImGui::TextUnformatted(id.c_str());
		ImGui::PopStyleColor();
		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		drawFields(index);
	}

	ImGui::End();
	ImGui::PopStyleVar();

	if (not open)
	{
		m_expanded = -1;
	}
}

void PDBehaviorEditor::drawResetModal()
{
	if (m_resetOpen)
	{
		m_resetOpen = false;
		ImGui::OpenPopup("Reset behaviors");
	}

	if (not beginModal("Reset behaviors", ImVec2(420.0f, 180.0f)))
	{
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
	ImGui::TextWrapped("Discard every edit for %s and go back to the values the pack ships with?", m_displayName.c_str());
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0.0f, 16.0f));

	if (ImGui::Button("Reset", ImVec2(110.0f, 32.0f)))
	{
		resetToPack();
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(110.0f, 32.0f)))
	{
		ImGui::CloseCurrentPopup();
	}

	endModal();
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
	ImGui::BeginDisabled(not m_hasOverride);

	if (ImGui::Button("Reset"))
	{
		m_resetOpen = true;
	}

	ImGui::EndDisabled();

	if (m_hasOverride and ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Discard every edit for this pack");
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
	drawResetModal();

	if (back)
	{
		return false;
	}

	if (not m_status.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::Stop);
		ImGui::TextWrapped("%s", m_status.c_str());
		ImGui::PopStyleColor();
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
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
