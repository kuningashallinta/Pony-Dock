#include <UI/Windows/PDMainWindow.h>

#include <App/PDMainApplication.h>
#include <Core/PDPaths.h>
#include <Core/PDString.h>
#include <UI/PDFileDialog.h>
#include <UI/PDImGui.h>
#include <UI/PDTheme.h>
#include <UI/PDWidgets.h>

#include <windows.h>

#include <shellapi.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <utility>

void PDMainWindow::addImageFitted(ImDrawList *drawList, PDTexture const *texture, ImVec2 areaMin, ImVec2 areaMax, float margin)
{
	if (texture == nullptr or not texture->valid() or texture->width() <= 0 or texture->height() <= 0)
	{
		return;
	}

	const float areaWidth = areaMax.x - areaMin.x - margin * 2.0f;
	const float areaHeight = areaMax.y - areaMin.y - margin * 2.0f;

	if (areaWidth <= 0.0f or areaHeight <= 0.0f)
	{
		return;
	}

	const float textureWidth = static_cast<float>(texture->width());
	const float textureHeight = static_cast<float>(texture->height());
	float scale = std::min(areaWidth / textureWidth, areaHeight / textureHeight);
	scale = std::min(scale, 2.0f);

	const float drawWidth = textureWidth * scale;
	const float drawHeight = textureHeight * scale;
	const float offsetX = areaMin.x + margin + (areaWidth - drawWidth) * 0.5f;
	const float offsetY = areaMin.y + margin + (areaHeight - drawHeight) * 0.5f;

	drawList->AddImage(
		reinterpret_cast<ImTextureID>(texture->view()),
		ImVec2(offsetX, offsetY),
		ImVec2(offsetX + drawWidth, offsetY + drawHeight));
}

void PDMainWindow::addShadow(ImDrawList *drawList, ImVec2 rectMin, ImVec2 rectMax)
{
	for (int index = 0; index < 6; index += 1)
	{
		const float spread = ShadowSpread * static_cast<float>(index + 1) / 6.0f;
		const float fade = 1.0f - static_cast<float>(index) / 6.0f;
		const ImVec4 color(PDTheme::Shadow.x, PDTheme::Shadow.y, PDTheme::Shadow.z, PDTheme::Shadow.w * fade);

		drawList->AddRect(
			ImVec2(rectMin.x - spread, rectMin.y - spread),
			ImVec2(rectMax.x + spread, rectMax.y + spread),
			ImGui::GetColorU32(color),
			0.0f,
			0,
			1.0f);
	}
}

PDMainWindow::PDMainWindow(PDMainApplication &app, PDImGui &host, PDDiagnostics &diagnostics)
	: m_app(app),
	  m_host(host),
	  m_diagnostics(diagnostics),
	  m_behaviorEditor(app, host, diagnostics)
{
	m_catalog.load(packsRoot());
	m_scripts.load(scriptsRoot());
	m_requiredScripts = PDMainApplication::requiredScripts();

	for (PDPonyGroup const &group : m_catalog.groups())
	{
		for (PDPonyPack const &variant : group.variants)
		{
			PDSettingTarget target;
			target.id = variant.id;
			target.label = group.displayName;
			target.previewPath = variant.previewPath;

			if (group.variants.size() > 1)
			{
				target.label += "  (" + variant.id + ")";
			}

			m_targets.push_back(std::move(target));
		}
	}
}

void PDMainWindow::draw()
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin(
		"Pony Dock",
		nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

	ImGui::PopStyleVar(3);

	const float height = ImGui::GetContentRegionAvail().y;
	drawSidebar(NavWidth, height);
	ImGui::SameLine(0.0f, 0.0f);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 18.0f));
	ImGui::BeginChild("content", ImVec2(0.0f, height), ImGuiChildFlags_AlwaysUseWindowPadding);
	drawContent();
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const ImVec2 windowPos = ImGui::GetWindowPos();
	drawList->AddLine(
		ImVec2(windowPos.x + NavWidth, windowPos.y),
		ImVec2(windowPos.x + NavWidth, windowPos.y + height),
		ImGui::GetColorU32(PDTheme::Divider),
		1.0f);

	ImGui::End();

	if (m_activeView == View::Behaviors)
	{
		m_behaviorEditor.drawDetail();
	}
}

void PDMainWindow::drawSidebar(float width, float height)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, PDTheme::Sidebar);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SidebarPad, SidebarPad));
	ImGui::BeginChild("sidebar", ImVec2(width, height), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleVar();

	navItem("Browser", View::Browser);
	ImGui::Dummy(ImVec2(0.0f, 2.0f));
	navItem("Scene", View::Scene);
	ImGui::Dummy(ImVec2(0.0f, 2.0f));
	navItem("Modules", View::Modules);
	ImGui::Dummy(ImVec2(0.0f, 2.0f));
	navItem("Settings", View::Settings);
	ImGui::Dummy(ImVec2(0.0f, 2.0f));
	navItem("Log", View::Log);

	const float spacing = ImGui::GetStyle().ItemSpacing.y;
	const float footerHeight = RunButtonHeight + FooterGap + QuitButtonHeight + spacing * 3.0f;
	const float remaining = ImGui::GetContentRegionAvail().y - footerHeight;

	if (remaining > 0.0f)
	{
		ImGui::Dummy(ImVec2(0.0f, remaining));
	}

	drawRunControl();

	ImGui::Dummy(ImVec2(0.0f, FooterGap));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextFaint);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, PDTheme::SidebarHover);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, PDTheme::SidebarHover);

	if (ImGui::Button("Quit Pony Dock", ImVec2(ImGui::GetContentRegionAvail().x, QuitButtonHeight)))
	{
		m_app.requestExit();
	}

	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();

	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void PDMainWindow::navItem(const char *label, View view)
{
	constexpr float itemHeight = 38.0f;
	const ImVec2 position = ImGui::GetCursorScreenPos();
	const ImVec2 size(ImGui::GetContentRegionAvail().x, itemHeight);

	ImGui::PushID(static_cast<int>(view));
	const bool pressed = ImGui::InvisibleButton("nav", size);
	const bool hovered = ImGui::IsItemHovered();
	ImGui::PopID();

	const bool active = m_activeView == view;
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const ImVec2 rectMax(position.x + size.x, position.y + size.y);

	if (active)
	{
		drawList->AddRectFilled(position, rectMax, ImGui::GetColorU32(PDTheme::AccentSoft), 0.0f);
		drawList->AddRectFilled(position, ImVec2(position.x + 3.0f, rectMax.y), ImGui::GetColorU32(PDTheme::Accent), 0.0f);
	}
	else if (hovered)
	{
		drawList->AddRectFilled(position, rectMax, ImGui::GetColorU32(PDTheme::SidebarHover), 0.0f);
	}

	const ImVec4 &textColor = active ? PDTheme::AccentText : (hovered ? PDTheme::White : PDTheme::TextDim);
	const float textY = position.y + (itemHeight - ImGui::GetTextLineHeight()) * 0.5f;
	drawList->AddText(ImVec2(position.x + 16.0f, textY), ImGui::GetColorU32(textColor), label);

	if (view == View::Scene)
	{
		const int total = sceneTotalQuantity();

		if (total > 0)
		{
			char badge[16];
			std::snprintf(badge, sizeof(badge), "%d", total);

			const ImVec2 badgeSize = ImGui::CalcTextSize(badge);
			constexpr float badgePadX = 8.0f;
			const ImVec2 badgeMax(rectMax.x - 10.0f, position.y + (itemHeight + badgeSize.y + 6.0f) * 0.5f);
			const ImVec2 badgeMin(badgeMax.x - badgeSize.x - badgePadX * 2.0f, position.y + (itemHeight - badgeSize.y - 6.0f) * 0.5f);

			drawList->AddRectFilled(badgeMin, badgeMax, ImGui::GetColorU32(active ? PDTheme::Accent : PDTheme::Badge), 0.0f);
			drawList->AddText(ImVec2(badgeMin.x + badgePadX, badgeMin.y + 3.0f), ImGui::GetColorU32(active ? PDTheme::White : PDTheme::TextDim), badge);
		}
	}

	if (pressed)
	{
		setView(view);
	}
}

void PDMainWindow::drawRunControl()
{
	const bool running = m_app.isRunning();
	const float width = ImGui::GetContentRegionAvail().x;

	if (running)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, PDTheme::Stop);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, PDTheme::StopHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, PDTheme::StopPress);

		if (ImGui::Button("Stop", ImVec2(width, RunButtonHeight)))
		{
			m_app.stopScene();
		}
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, PDTheme::Start);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, PDTheme::StartHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, PDTheme::StartPress);

		ImGui::BeginDisabled(m_scene.empty());

		if (ImGui::Button("Start", ImVec2(width, RunButtonHeight)))
		{
			m_app.startScene(m_scene);
		}

		ImGui::EndDisabled();

		if (m_scene.empty() and ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Add ponies from Browser first");
		}
	}

	ImGui::PopStyleColor(3);
}

void PDMainWindow::setView(View view)
{
	m_activeView = view;
}

void PDMainWindow::drawContent()
{
	switch (m_activeView)
	{
		case View::Browser:
		{
			drawBrowserView();

			break;
		}

		case View::Scene:
		{
			drawSceneView();

			break;
		}

		case View::Modules:
		{
			drawModulesView();

			break;
		}

		case View::Settings:
		{
			drawSettingsView();

			break;
		}

		case View::Behaviors:
		{
			if (not m_behaviorEditor.draw())
			{
				m_behaviorEditor.close();
				m_activeView = m_returnView;
			}

			break;
		}

		case View::Log:
		{
			drawLogView();

			break;
		}

		default:
		{
			break;
		}
	}
}

void PDMainWindow::drawBrowserView()
{
	m_thumbnailBudget = 24;

	beginToolbar();

	ImGui::SetNextItemWidth(250.0f);
	ImGui::InputTextWithHint("##search", "Search ponies", m_search, sizeof(m_search));

	endToolbar();

	ImGui::BeginChild("grid");

	std::string const query = toLower(m_search);
	std::vector<PDPonyGroup> const &groups = m_catalog.groups();

	const float available = ImGui::GetContentRegionAvail().x;
	const int columns = std::max(1, static_cast<int>((available + CardSpacing) / (CardWidth + CardSpacing)));

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(CardSpacing, CardSpacing));

	int shown = 0;

	for (std::size_t index = 0; index < groups.size(); index += 1)
	{
		PDPonyGroup const &group = groups[index];

		if (not query.empty() and toLower(group.displayName).find(query) == std::string::npos)
		{
			continue;
		}

		if (shown % columns != 0)
		{
			ImGui::SameLine();
		}

		shown += 1;

		std::string sub;

		if (group.variants.size() > 1)
		{
			char buffer[32];
			std::snprintf(buffer, sizeof(buffer), "%zu variants", group.variants.size());
			sub = buffer;
		}

		ImGui::PushID(static_cast<int>(index));

		PDCardInput const input = drawPonyCard(
			group.displayName,
			sub,
			thumbnail(group.variants.front().previewPath),
			m_selectedGroup == static_cast<int>(index));

		if (input.clicked)
		{
			m_selectedGroup = static_cast<int>(index);
		}

		if (input.doubleClicked)
		{
			openGroup(index, VariantStage);
		}

		if (input.rightClicked)
		{
			m_selectedGroup = static_cast<int>(index);
			openGroup(index, VariantEdit);
		}

		ImGui::PopID();
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();

	drawVariantModal();
}

void PDMainWindow::openGroup(std::size_t index, int action)
{
	PDPonyGroup const &group = m_catalog.groups()[index];
	m_variantAction = action;

	if (group.variants.size() > 1)
	{
		m_variantGroup = static_cast<int>(index);
		m_variantOpen = true;

		return;
	}

	applyVariant(group, group.variants.front());
}

void PDMainWindow::applyVariant(PDPonyGroup const &group, PDPonyPack const &variant)
{
	if (m_variantAction == VariantEdit)
	{
		openBehaviorEditor(variant.packPath, group.displayName);

		return;
	}

	addToScene(group.displayName, variant);
}

void PDMainWindow::openBehaviorEditor(std::string const &packPath, std::string const &displayName)
{
	if (not m_behaviorEditor.open(packPath, displayName))
	{
		m_diagnostics.write("Cannot open the behavior editor for " + packPath);

		return;
	}

	m_returnView = m_activeView;
	m_activeView = View::Behaviors;
}

PDCardInput PDMainWindow::drawPonyCard(
	std::string const &name,
	std::string const &sub,
	PDTexture const *texture,
	bool selected)
{
	const float nameHeight = ImGui::GetTextLineHeight();
	const float subHeight = nameHeight * 0.85f;
	const float cardHeight = CardPad + ThumbHeight + 7.0f + nameHeight + subHeight + CardPad;

	const ImVec2 position = ImGui::GetCursorScreenPos();
	const ImVec2 size(CardWidth, cardHeight);
	const ImVec2 rectMax(position.x + size.x, position.y + size.y);

	PDCardInput input;
	input.clicked = ImGui::InvisibleButton("card", size);

	const bool hovered = ImGui::IsItemHovered();
	input.doubleClicked = hovered and ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
	input.rightClicked = hovered and ImGui::IsMouseClicked(ImGuiMouseButton_Right);

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(position, rectMax, ImGui::GetColorU32(hovered ? PDTheme::CardBgHover : PDTheme::CardBg), 0.0f);

	const ImVec2 thumbMin(position.x + CardPad, position.y + CardPad);
	const ImVec2 thumbMax(rectMax.x - CardPad, position.y + CardPad + ThumbHeight);
	drawList->AddRectFilled(thumbMin, thumbMax, ImGui::GetColorU32(PDTheme::ThumbBg), 0.0f);
	drawList->AddRect(thumbMin, thumbMax, ImGui::GetColorU32(PDTheme::CardBorder), 0.0f, 0, 1.0f);
	addImageFitted(drawList, texture, thumbMin, thumbMax, 6.0f);

	drawList->AddText(ImVec2(position.x + CardPad + 2.0f, thumbMax.y + 7.0f), ImGui::GetColorU32(PDTheme::White), name.c_str());

	if (not sub.empty())
	{
		drawList->AddText(ImVec2(position.x + CardPad + 2.0f, thumbMax.y + 7.0f + nameHeight + 2.0f), ImGui::GetColorU32(PDTheme::TextFaint), sub.c_str());
	}

	drawList->AddRect(position, rectMax, ImGui::GetColorU32(selected ? PDTheme::Accent : PDTheme::CardBorder), 0.0f, 0, selected ? 2.0f : 1.0f);

	return input;
}

PDTexture *PDMainWindow::thumbnail(std::string const &path)
{
	if (path.empty())
	{
		return nullptr;
	}

	const auto cached = m_thumbnails.find(path);

	if (cached != m_thumbnails.end())
	{
		return &cached->second;
	}

	if (m_thumbnailBudget <= 0)
	{
		return nullptr;
	}

	m_thumbnailBudget -= 1;

	return &m_thumbnails.emplace(path, PDTexture(m_host.device(), path)).first->second;
}

void PDMainWindow::drawSceneView()
{
	beginToolbar();

	if (ImGui::Button("Clear scene"))
	{
		m_scene.clear();
	}

	char summary[64];
	std::snprintf(summary, sizeof(summary), "%d across %zu packs", sceneTotalQuantity(), m_scene.size());
	toolbarSummary(summary);

	endToolbar();

	if (m_scene.empty())
	{
		ImGui::Dummy(ImVec2(0.0f, 40.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);

		char const *const line1 = "Nothing staged yet";
		char const *const line2 = "Double-click a pack in Browser to add it here.";
		float width = ImGui::CalcTextSize(line1).x;
		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - width) * 0.5f);
		ImGui::TextUnformatted(line1);

		width = ImGui::CalcTextSize(line2).x;
		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - width) * 0.5f);
		ImGui::TextUnformatted(line2);

		ImGui::PopStyleColor();

		return;
	}

	constexpr float rowHeight = 56.0f;
	std::size_t removeIndex = m_scene.size();

	for (std::size_t index = 0; index < m_scene.size(); index += 1)
	{
		PDSceneEntry &entry = m_scene[index];

		ImGui::PushID(static_cast<int>(index));

		const ImVec2 rowMin = ImGui::GetCursorScreenPos();
		const ImVec2 rowSize(ImGui::GetContentRegionAvail().x, rowHeight);
		const ImVec2 rowMax(rowMin.x + rowSize.x, rowMin.y + rowSize.y);

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(PDTheme::CardBg), 0.0f);
		drawList->AddRect(rowMin, rowMax, ImGui::GetColorU32(PDTheme::CardBorder), 0.0f, 0, 1.0f);

		const ImVec2 thumbMin(rowMin.x + 8.0f, rowMin.y + 8.0f);
		const ImVec2 thumbMax(thumbMin.x + 40.0f, thumbMin.y + 40.0f);
		drawList->AddRectFilled(thumbMin, thumbMax, ImGui::GetColorU32(PDTheme::ThumbBg), 0.0f);
		drawList->AddRect(thumbMin, thumbMax, ImGui::GetColorU32(PDTheme::CardBorder), 0.0f, 0, 1.0f);
		addImageFitted(drawList, thumbnail(entry.previewPath), thumbMin, thumbMax, 3.0f);

		drawList->AddText(ImVec2(thumbMax.x + 12.0f, rowMin.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f), ImGui::GetColorU32(PDTheme::White), entry.displayName.c_str());

		const float stepperWidth = 24.0f + 32.0f + 24.0f;
		const float editWidth = ImGui::CalcTextSize("Edit").x + 20.0f;
		const float removeWidth = ImGui::CalcTextSize("Remove").x + 24.0f;
		const float controlsWidth = stepperWidth + 10.0f + editWidth + 10.0f + removeWidth;

		ImGui::SetCursorScreenPos(ImVec2(rowMax.x - controlsWidth - 10.0f, rowMin.y + (rowHeight - 24.0f) * 0.5f));

		bool removed = false;

		if (ImGui::Button("-", ImVec2(24.0f, 24.0f)))
		{
			if (entry.quantity > 1)
			{
				entry.quantity -= 1;
			}
			else
			{
				removed = true;
			}
		}

		ImGui::SameLine(0.0f, 0.0f);

		char quantity[16];
		std::snprintf(quantity, sizeof(quantity), "%d", entry.quantity);
		const float quantityWidth = ImGui::CalcTextSize(quantity).x;
		const ImVec2 quantityPos = ImGui::GetCursorScreenPos();
		drawList->AddText(ImVec2(quantityPos.x + (32.0f - quantityWidth) * 0.5f, quantityPos.y + 4.0f), ImGui::GetColorU32(PDTheme::White), quantity);
		ImGui::Dummy(ImVec2(32.0f, 24.0f));
		ImGui::SameLine(0.0f, 0.0f);

		if (ImGui::Button("+", ImVec2(24.0f, 24.0f)))
		{
			entry.quantity += 1;
		}

		ImGui::SameLine(0.0f, 10.0f);

		if (ImGui::Button("Edit", ImVec2(editWidth, 24.0f)))
		{
			openBehaviorEditor(entry.packPath, entry.displayName);
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Edit this pack's behaviors");
		}

		ImGui::SameLine(0.0f, 10.0f);

		if (ImGui::Button("Remove", ImVec2(removeWidth, 24.0f)))
		{
			removed = true;
		}

		ImGui::SetCursorScreenPos(rowMin);
		ImGui::Dummy(ImVec2(rowSize.x, rowHeight + 8.0f));

		ImGui::PopID();

		if (removed)
		{
			removeIndex = index;
		}
	}

	if (removeIndex < m_scene.size())
	{
		removeSceneEntry(removeIndex);
	}
}

void PDMainWindow::addToScene(std::string const &displayName, PDPonyPack const &pack)
{
	for (PDSceneEntry &entry : m_scene)
	{
		if (entry.id == pack.id)
		{
			entry.quantity += 1;

			return;
		}
	}

	PDSceneEntry entry;
	entry.id = pack.id;
	entry.displayName = displayName;
	entry.previewPath = pack.previewPath;
	entry.packPath = pack.packPath;
	entry.quantity = 1;
	m_scene.push_back(std::move(entry));
}

void PDMainWindow::removeSceneEntry(std::size_t index)
{
	if (index >= m_scene.size())
	{
		return;
	}

	m_scene.erase(m_scene.begin() + static_cast<std::ptrdiff_t>(index));
}

void PDMainWindow::drawModulesView()
{
	if (not m_settingsModule.empty())
	{
		drawModuleSettings();

		return;
	}

	beginToolbar();

	if (ImGui::Button("Rescan"))
	{
		m_scripts.load(scriptsRoot());
	}

	ImGui::SameLine();

	if (ImGui::Button("Reload scripts"))
	{
		m_app.reloadScripts();
	}

	char summary[64];
	std::snprintf(summary, sizeof(summary), "%zu scripts", m_scripts.entries().size());
	toolbarSummary(summary);

	endToolbar();

	if (m_scripts.entries().empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
		ImGui::TextUnformatted("No scripts found.");
		ImGui::PopStyleColor();

		return;
	}

	m_loadedScripts = m_app.loadedScripts();

	const float columnWidth = (ImGui::GetContentRegionAvail().x - ColumnGap) * 0.5f;

	drawScriptColumn("LOADED", true, columnWidth);
	ImGui::SameLine(0.0f, ColumnGap);
	drawScriptColumn("UNLOADED", false, columnWidth);
}

void PDMainWindow::drawScriptColumn(const char *label, bool loadedColumn, float width)
{
	int count = 0;

	for (PDScriptEntry const &entry : m_scripts.entries())
	{
		bool const loaded = std::find(m_loadedScripts.begin(), m_loadedScripts.end(), entry.fullPath) != m_loadedScripts.end();

		if (loaded == loadedColumn)
		{
			count += 1;
		}
	}

	ImGui::BeginChild(label, ImVec2(width, 0.0f));
	drawColumnHeader(label, count);

	for (std::size_t index = 0; index < m_scripts.entries().size(); index += 1)
	{
		PDScriptEntry const &entry = m_scripts.entries()[index];
		bool const loaded = std::find(m_loadedScripts.begin(), m_loadedScripts.end(), entry.fullPath) != m_loadedScripts.end();

		if (loaded != loadedColumn)
		{
			continue;
		}

		ImGui::PushID(static_cast<int>(index));
		drawScriptCard(entry, loaded);
		ImGui::PopID();
	}

	ImGui::EndChild();
}

void PDMainWindow::drawColumnHeader(const char *label, int count)
{
	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
	ImGui::Text("%s  (%d)", label, count);
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

void PDMainWindow::drawScriptCard(PDScriptEntry const &entry, bool loaded)
{
	const bool required = std::find(m_requiredScripts.begin(), m_requiredScripts.end(), entry.fullPath) != m_requiredScripts.end();
	const char *const requiredLabel = "Required";

	const ImVec2 position = ImGui::GetCursorScreenPos();
	const ImVec2 size(ImGui::GetContentRegionAvail().x, ScriptRowHeight);
	const ImVec2 rectMax(position.x + size.x, position.y + size.y);

	ImGui::SetNextItemAllowOverlap();
	const bool clicked = ImGui::InvisibleButton("script", size);
	const bool hovered = ImGui::IsItemHovered();

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(position, rectMax, ImGui::GetColorU32(hovered ? PDTheme::CardBgHover : PDTheme::CardBg), 0.0f);
	drawList->AddRect(position, rectMax, ImGui::GetColorU32(PDTheme::CardBorder), 0.0f, 0, 1.0f);

	const float controlLeft = rectMax.x - RowPad - ToggleWidth;
	const float textY = position.y + (ScriptRowHeight - ImGui::GetTextLineHeight()) * 0.5f;

	drawList->AddText(
		ImVec2(position.x + RowPad, textY),
		ImGui::GetColorU32(PDTheme::White),
		entry.relativePath.c_str());

	char lines[32];
	std::snprintf(lines, sizeof(lines), "%d lines", entry.lineCount);
	const float linesWidth = ImGui::CalcTextSize(lines).x;

	drawList->AddText(
		ImVec2(controlLeft - RowPad - linesWidth, textY),
		ImGui::GetColorU32(PDTheme::TextFaint),
		lines);

	if (required)
	{
		const ImVec2 labelSize = ImGui::CalcTextSize(requiredLabel);
		const ImVec2 chipMin(controlLeft, position.y + (ScriptRowHeight - ToggleHeight) * 0.5f);
		const ImVec2 chipMax(chipMin.x + ToggleWidth, chipMin.y + ToggleHeight);

		drawList->AddRectFilled(chipMin, chipMax, ImGui::GetColorU32(PDTheme::AccentSoft), 0.0f);

		drawList->AddText(
			ImVec2(chipMin.x + (ToggleWidth - labelSize.x) * 0.5f, chipMin.y + (ToggleHeight - labelSize.y) * 0.5f),
			ImGui::GetColorU32(PDTheme::AccentText),
			requiredLabel);
	}
	else
	{
		ImGui::SetCursorScreenPos(ImVec2(controlLeft, position.y + (ScriptRowHeight - ToggleHeight) * 0.5f));

		if (loaded)
		{
			if (ImGui::Button("Unload", ImVec2(ToggleWidth, ToggleHeight)))
			{
				m_app.setScriptLoaded(entry.fullPath, false);
			}
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, PDTheme::Accent);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, PDTheme::AccentHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, PDTheme::AccentPress);

			if (ImGui::Button("Load", ImVec2(ToggleWidth, ToggleHeight)))
			{
				m_app.setScriptLoaded(entry.fullPath, true);
			}

			ImGui::PopStyleColor(3);
		}
	}

	if (clicked)
	{
		openModule(entry);
	}

	ImGui::SetCursorScreenPos(position);
	ImGui::Dummy(ImVec2(size.x, ScriptRowHeight + 4.0f));
}

void PDMainWindow::openModule(PDScriptEntry const &entry)
{
	m_settingsModule = entry.relativePath;
	m_settingsModulePath = entry.fullPath;
	m_settingsTarget.clear();
	m_settingsTargetLabel.clear();
	m_targetFilter[0] = '\0';
}

void PDMainWindow::openInEditor()
{
	std::wstring const path = std::filesystem::path(m_settingsModulePath).wstring();
	HINSTANCE result = ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

	if (reinterpret_cast<INT_PTR>(result) > 32)
	{
		return;
	}

	result = ShellExecuteW(nullptr, L"openas", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

	if (reinterpret_cast<INT_PTR>(result) > 32)
	{
		return;
	}

	m_diagnostics.write("Cannot open " + m_settingsModulePath + " in an editor");
}

int PDMainWindow::gridColumns(float available, float &outIndent)
{
	const int columns = std::max(1, static_cast<int>((available + CardSpacing) / (CardWidth + CardSpacing)));
	const float blockWidth = static_cast<float>(columns) * CardWidth + static_cast<float>(columns - 1) * CardSpacing;
	outIndent = std::max(0.0f, (available - blockWidth) * 0.5f);

	return columns;
}

void PDMainWindow::drawVariantModal()
{
	if (m_variantOpen)
	{
		m_variantOpen = false;
		ImGui::OpenPopup("Choose variant");
	}

	if (not beginModal("Choose variant", ImVec2(560.0f, 420.0f)))
	{
		return;
	}

	std::vector<PDPonyGroup> const &groups = m_catalog.groups();

	if (m_variantGroup < 0 or m_variantGroup >= static_cast<int>(groups.size()))
	{
		ImGui::CloseCurrentPopup();
		endModal();

		return;
	}

	PDPonyGroup const &group = groups[static_cast<std::size_t>(m_variantGroup)];
	m_thumbnailBudget = 24;

	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
	ImGui::Text("%s ships %zu variants.", group.displayName.c_str(), group.variants.size());
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	ImGui::BeginChild("variantGrid", ImVec2(0.0f, -46.0f), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);

	float indent = 0.0f;
	const int columns = gridColumns(ImGui::GetContentRegionAvail().x, indent);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(CardSpacing, CardSpacing));

	int chosen = -1;

	for (std::size_t index = 0; index < group.variants.size(); index += 1)
	{
		if (index % static_cast<std::size_t>(columns) != 0)
		{
			ImGui::SameLine();
		}
		else
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
		}

		PDPonyPack const &variant = group.variants[index];
		ImGui::PushID(static_cast<int>(index));

		PDCardInput const input = drawPonyCard(variant.id, std::string(), thumbnail(variant.previewPath), false);

		ImGui::PopID();

		if (input.clicked)
		{
			chosen = static_cast<int>(index);
		}
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	if (ImGui::Button("Cancel", ImVec2(110.0f, 32.0f)))
	{
		ImGui::CloseCurrentPopup();
	}

	if (chosen >= 0)
	{
		applyVariant(group, group.variants[static_cast<std::size_t>(chosen)]);
		ImGui::CloseCurrentPopup();
	}

	endModal();
}

void PDMainWindow::drawTargetButton()
{
	std::string const label = m_settingsTarget.empty() ? std::string("Global defaults") : m_settingsTargetLabel;

	if (ImGui::Button(label.c_str(), ImVec2(TargetPickerWidth, 0.0f)))
	{
		m_targetPickerOpen = true;
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Choose which pack these settings apply to");
	}
}

void PDMainWindow::drawTargetModal()
{
	if (m_targetPickerOpen)
	{
		m_targetFilter[0] = '\0';
		m_targetPickerOpen = false;
		ImGui::OpenPopup("Choose target");
	}

	if (not beginModal("Choose target", ImVec2(560.0f, 460.0f)))
	{
		return;
	}

	m_thumbnailBudget = 24;

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##targetFilter", "Search packs", m_targetFilter, sizeof(m_targetFilter));
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	std::string const filter = toLower(m_targetFilter);
	std::vector<std::string> const overridden = m_app.settings().packsWithOverrides(m_settingsModule);

	std::vector<int> visible;

	if (filter.empty())
	{
		visible.push_back(-1);
	}

	for (std::size_t index = 0; index < m_targets.size(); index += 1)
	{
		if (filter.empty() or toLower(m_targets[index].label).find(filter) != std::string::npos)
		{
			visible.push_back(static_cast<int>(index));
		}
	}

	ImGui::BeginChild("targetGrid", ImVec2(0.0f, -46.0f), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);

	float indent = 0.0f;
	const int columns = gridColumns(ImGui::GetContentRegionAvail().x, indent);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(CardSpacing, CardSpacing));

	bool chosen = false;
	std::string chosenId;
	std::string chosenLabel;

	for (std::size_t slot = 0; slot < visible.size(); slot += 1)
	{
		if (slot % static_cast<std::size_t>(columns) != 0)
		{
			ImGui::SameLine();
		}
		else
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
		}

		const int index = visible[slot];
		ImGui::PushID(index);

		PDCardInput input;

		if (index < 0)
		{
			input = drawPonyCard("Global defaults", "every pack", nullptr, m_settingsTarget.empty());
		}
		else
		{
			PDSettingTarget const &target = m_targets[static_cast<std::size_t>(index)];
			const bool hasOverride = std::find(overridden.begin(), overridden.end(), target.id) != overridden.end();

			input = drawPonyCard(
				target.label,
				hasOverride ? "overridden" : std::string(),
				thumbnail(target.previewPath),
				m_settingsTarget == target.id);
		}

		ImGui::PopID();

		if (input.clicked)
		{
			chosen = true;

			if (index >= 0)
			{
				chosenId = m_targets[static_cast<std::size_t>(index)].id;
				chosenLabel = m_targets[static_cast<std::size_t>(index)].label;
			}
		}
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();

	if (visible.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
		ImGui::TextUnformatted("No packs match that search.");
		ImGui::PopStyleColor();
	}

	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	if (ImGui::Button("Cancel", ImVec2(110.0f, 32.0f)))
	{
		ImGui::CloseCurrentPopup();
	}

	if (chosen)
	{
		m_settingsTarget = chosenId;
		m_settingsTargetLabel = chosenLabel;
		ImGui::CloseCurrentPopup();
	}

	endModal();
}

void PDMainWindow::drawSettingRow(PDSettingDeclaration const &declaration, bool loaded)
{
	PDSettingsStore &store = m_app.settings();

	const bool packScope = not m_settingsTarget.empty();
	const bool overridden = packScope and store.hasOverride(m_settingsModule, m_settingsTarget, declaration.id);
	const bool isButton = declaration.kind == PDSettingKind::Button;
	const bool editable = isButton or not packScope or overridden;

	const ImVec2 position = ImGui::GetCursorScreenPos();
	const ImVec2 size(ImGui::GetContentRegionAvail().x, SettingRowHeight);
	const ImVec2 rectMax(position.x + size.x, position.y + size.y);

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(position, rectMax, ImGui::GetColorU32(PDTheme::CardBg), 0.0f);
	drawList->AddRect(position, rectMax, ImGui::GetColorU32(PDTheme::CardBorder), 0.0f, 0, 1.0f);

	const float lineHeight = ImGui::GetTextLineHeight();
	const float firstY = position.y + (SettingRowHeight - lineHeight * 2.0f - 2.0f) * 0.5f;

	drawList->AddText(
		ImVec2(position.x + RowPad, firstY),
		ImGui::GetColorU32(PDTheme::White),
		declaration.label.c_str());

	drawList->AddText(
		ImVec2(position.x + RowPad, firstY + lineHeight + 2.0f),
		ImGui::GetColorU32(PDTheme::TextFaint),
		declaration.id.c_str());

	const float controlHeight = ImGui::GetFrameHeight();
	const float controlY = position.y + (SettingRowHeight - controlHeight) * 0.5f;
	const float controlLeft = rectMax.x - RowPad - SettingControlWidth;
	const float markerLeft = controlLeft - 8.0f - controlHeight;

	PDSettingValue value = store.value(m_settingsModule, m_settingsTarget, declaration.id);
	bool changed = false;
	bool commit = false;

	ImGui::SetCursorScreenPos(ImVec2(controlLeft, controlY));
	ImGui::SetNextItemWidth(SettingControlWidth);
	ImGui::BeginDisabled(not editable);

	switch (declaration.kind)
	{
		case PDSettingKind::Checkbox:
		{
			bool current = value.boolean;

			if (ImGui::Checkbox("##value", &current))
			{
				value.type = PDSettingValueType::Boolean;
				value.boolean = current;
				changed = true;
				commit = true;
			}

			break;
		}

		case PDSettingKind::Slider:
		{
			float current = value.number;

			if (ImGui::SliderFloat("##value", &current, declaration.minimum, declaration.maximum, "%.2f"))
			{
				value.type = PDSettingValueType::Number;
				value.number = current;
				changed = true;
			}

			commit = ImGui::IsItemDeactivatedAfterEdit();

			break;
		}

		case PDSettingKind::Text:
		{
			std::string current = value.text;

			if (ImGui::InputText("##value", &current))
			{
				value.type = PDSettingValueType::Text;
				value.text = current;
				changed = true;
			}

			commit = ImGui::IsItemDeactivatedAfterEdit();

			break;
		}

		case PDSettingKind::Dropdown:
		{
			std::vector<const char *> items;
			items.reserve(declaration.options.size());
			int current = 0;

			for (std::size_t index = 0; index < declaration.options.size(); index += 1)
			{
				items.push_back(declaration.options[index].c_str());

				if (declaration.options[index] == value.text)
				{
					current = static_cast<int>(index);
				}
			}

			if (not items.empty() and ImGui::Combo("##value", &current, items.data(), static_cast<int>(items.size())))
			{
				value.type = PDSettingValueType::Text;
				value.text = declaration.options[static_cast<std::size_t>(current)];
				changed = true;
				commit = true;
			}

			break;
		}

		case PDSettingKind::Button:
		{
			ImGui::BeginDisabled(not loaded);

			if (ImGui::Button("Run", ImVec2(SettingControlWidth, controlHeight)))
			{
				m_app.pressSettingButton(m_settingsModule, declaration.id);
			}

			ImGui::EndDisabled();

			if (not loaded and ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Load this module to run its actions");
			}

			break;
		}

		default:
		{
			break;
		}
	}

	ImGui::EndDisabled();

	if (not isButton)
	{
		ImGui::SetCursorScreenPos(ImVec2(markerLeft, controlY));

		if (packScope)
		{
			bool current = overridden;

			if (ImGui::Checkbox("##override", &current))
			{
				if (current)
				{
					store.setValue(m_settingsModule, m_settingsTarget, declaration.id, value);
				}
				else
				{
					store.clearValue(m_settingsModule, m_settingsTarget, declaration.id);
				}

				store.save();
			}

			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Override this setting for %s", m_settingsTargetLabel.c_str());
			}
		}
		else if (store.hasStoredValue(m_settingsModule, declaration.id))
		{
			if (ImGui::Button("x", ImVec2(controlHeight, controlHeight)))
			{
				store.clearValue(m_settingsModule, std::string(), declaration.id);
				store.save();
			}

			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Reset to the script default");
			}
		}
	}

	if (changed)
	{
		store.setValue(m_settingsModule, m_settingsTarget, declaration.id, value);
	}

	if (commit)
	{
		store.save();
	}

	ImGui::SetCursorScreenPos(position);
	ImGui::Dummy(ImVec2(size.x, SettingRowHeight + 4.0f));
}

void PDMainWindow::drawModuleSettings()
{
	bool back = false;

	beginToolbar();

	if (ImGui::Button("Back"))
	{
		back = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("Open in editor"))
	{
		openInEditor();
	}

	ImGui::SameLine();
	drawTargetButton();
	toolbarSummary(m_settingsModule.c_str());
	endToolbar();

	drawTargetModal();

	if (back)
	{
		m_settingsModule.clear();
		m_settingsModulePath.clear();

		return;
	}

	m_loadedScripts = m_app.loadedScripts();

	const bool loaded = std::find(m_loadedScripts.begin(), m_loadedScripts.end(), m_settingsModulePath) != m_loadedScripts.end();
	std::vector<PDSettingDeclaration> const declarations = m_app.settings().declarations(m_settingsModule);

	const ImVec2 panelMin = ImGui::GetCursorScreenPos();
	const ImVec2 panelSize = ImGui::GetContentRegionAvail();
	const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);

	addShadow(ImGui::GetWindowDrawList(), panelMin, panelMax);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, PDTheme::Toolbar);
	ImGui::PushStyleColor(ImGuiCol_Border, PDTheme::CardBorder);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
	ImGui::BeginChild("settingsPanel", panelSize, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

	if (declarations.empty() and loaded)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
		ImGui::TextWrapped("This module does not declare any settings.");
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		ImGui::TextWrapped("Add one at the top of the script:");
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::AccentText);
		ImGui::TextWrapped("settings.slider(\"speed\", \"Speed\", 1.0, 0.1, 4.0)");
		ImGui::PopStyleColor();
	}
	else if (declarations.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
		ImGui::TextWrapped("Load this module to see the settings it declares.");
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		if (ImGui::Button("Load", ImVec2(ToggleWidth, 0.0f)))
		{
			m_app.setScriptLoaded(m_settingsModulePath, true);
		}
	}

	for (PDSettingDeclaration const &declaration : declarations)
	{
		ImGui::PushID(declaration.id.c_str());
		drawSettingRow(declaration, loaded);
		ImGui::PopID();
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
}

void PDMainWindow::drawSettingsView()
{
	if (m_monitors.empty())
	{
		m_monitors = enumerateMonitors();
	}

	PDSettingsStore &store = m_app.settings();

	beginToolbar();

	if (ImGui::Button("Rescan displays"))
	{
		m_monitors = enumerateMonitors();
	}

	char summary[64];
	std::snprintf(summary, sizeof(summary), "%zu displays", m_monitors.size());
	toolbarSummary(summary);

	endToolbar();

	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::White);
	ImGui::TextUnformatted("Walkable displays");
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
	ImGui::TextWrapped("Entities only walk on the displays you tick here. Unticking every display leaves the primary one walkable.");
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	for (PDMonitor const &monitor : m_monitors)
	{
		std::string const key = "monitor." + monitor.device;
		bool walkable = store.appFlag(key, true);

		const ImVec2 position = ImGui::GetCursorScreenPos();
		const ImVec2 size(ImGui::GetContentRegionAvail().x, SettingRowHeight);
		const ImVec2 rectMax(position.x + size.x, position.y + size.y);

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(position, rectMax, ImGui::GetColorU32(PDTheme::CardBg), 0.0f);
		drawList->AddRect(position, rectMax, ImGui::GetColorU32(PDTheme::CardBorder), 0.0f, 0, 1.0f);

		const float lineHeight = ImGui::GetTextLineHeight();
		const float firstY = position.y + (SettingRowHeight - lineHeight * 2.0f - 2.0f) * 0.5f;

		char geometry[96];
		std::snprintf(geometry, sizeof(geometry), "%s  at  %d, %d", monitor.device.c_str(), monitor.x, monitor.y);

		drawList->AddText(ImVec2(position.x + RowPad, firstY), ImGui::GetColorU32(PDTheme::White), monitor.label.c_str());
		drawList->AddText(ImVec2(position.x + RowPad, firstY + lineHeight + 2.0f), ImGui::GetColorU32(PDTheme::TextFaint), geometry);

		const float controlHeight = ImGui::GetFrameHeight();
		ImGui::SetCursorScreenPos(ImVec2(rectMax.x - RowPad - controlHeight, position.y + (SettingRowHeight - controlHeight) * 0.5f));
		ImGui::PushID(monitor.device.c_str());

		if (ImGui::Checkbox("##walkable", &walkable))
		{
			store.setAppFlag(key, walkable);
			store.save();
		}

		ImGui::PopID();
		ImGui::SetCursorScreenPos(position);
		ImGui::Dummy(ImVec2(size.x, SettingRowHeight + 4.0f));
	}

	ImGui::Dummy(ImVec2(0.0f, 18.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::White);
	ImGui::TextUnformatted("Configuration");
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
	ImGui::TextWrapped("Module settings, pack overrides and walkable displays are stored here:");
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextFaint);
	ImGui::TextWrapped("%s", store.path().c_str());
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	if (ImGui::Button("Export...", ImVec2(120.0f, 32.0f)))
	{
		exportConfig();
	}

	ImGui::SameLine();

	if (ImGui::Button("Import...", ImVec2(120.0f, 32.0f)))
	{
		importConfig();
	}

	if (not m_configStatus.empty())
	{
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, m_configFailed ? PDTheme::Stop : PDTheme::AccentText);
		ImGui::TextWrapped("%s", m_configStatus.c_str());
		ImGui::PopStyleColor();
	}
}

void PDMainWindow::exportConfig()
{
	std::string path;

	if (not saveFileDialog(m_host.handle(), "Export Pony Dock config", "pony-dock-config.json", path))
	{
		return;
	}

	m_configFailed = not m_app.settings().exportTo(path);
	m_configStatus = m_configFailed ? ("Could not write " + path) : ("Exported to " + path);
	m_diagnostics.write(m_configStatus);
}

void PDMainWindow::importConfig()
{
	std::string path;

	if (not openFileDialog(m_host.handle(), "Import Pony Dock config", path))
	{
		return;
	}

	m_configFailed = not m_app.settings().importFrom(path);

	m_configStatus = m_configFailed ? ("Could not read " + path) : ("Imported from " + path);

	m_diagnostics.write(m_configStatus);
}

void PDMainWindow::drawLogView()
{
	std::vector<std::string> const lines = m_diagnostics.lines();
	std::vector<std::pair<std::string, int>> const repeats = m_diagnostics.repeats();

	beginToolbar();

	if (ImGui::Button("Clear"))
	{
		m_diagnostics.clear();
	}

	char summary[64];
	std::snprintf(summary, sizeof(summary), "%zu lines", lines.size());
	toolbarSummary(summary);

	endToolbar();

	if (not repeats.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
		ImGui::TextUnformatted("Repeated");
		ImGui::PopStyleColor();

		for (std::pair<std::string, int> const &entry : repeats)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::AccentText);
			ImGui::Text("%d x", entry.second);
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::TextWrapped("%s", entry.first.c_str());
		}

		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
	}

	const ImVec2 panelMin = ImGui::GetCursorScreenPos();
	const ImVec2 panelSize = ImGui::GetContentRegionAvail();
	const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);

	addShadow(ImGui::GetWindowDrawList(), panelMin, panelMax);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, PDTheme::Toolbar);
	ImGui::PushStyleColor(ImGuiCol_Border, PDTheme::CardBorder);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
	ImGui::BeginChild("logLines", panelSize, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

	if (lines.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
		ImGui::TextUnformatted("Nothing logged yet.");
		ImGui::PopStyleColor();
	}

	for (std::string const &line : lines)
	{
		ImGui::TextWrapped("%s", line.c_str());
	}

	if (not lines.empty() and ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
}

int PDMainWindow::sceneTotalQuantity() const
{
	int total = 0;

	for (PDSceneEntry const &entry : m_scene)
	{
		total += entry.quantity;
	}

	return total;
}
