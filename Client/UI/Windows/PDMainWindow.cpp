#include <UI/Windows/PDMainWindow.h>

#include <App/PDMainApplication.h>
#include <Core/PDString.h>
#include <UI/PDImGui.h>
#include <UI/PDTheme.h>

#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iterator>
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
	  m_diagnostics(diagnostics)
{
	m_catalog.load(PONYDOCK_PACKS_DIR);
	m_scripts.load(PONYDOCK_SCRIPTS_DIR);
	m_requiredScripts = PDMainApplication::requiredScripts();
}

void PDMainWindow::draw()
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Pony Dock", nullptr, flags);
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
}

void PDMainWindow::drawSidebar(float width, float height)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, PDTheme::Sidebar);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 14.0f));
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

	constexpr float bottomGap = 20.0f;
	constexpr float footerHeight = 42.0f + 8.0f + 30.0f + bottomGap;
	const float remaining = ImGui::GetContentRegionAvail().y - footerHeight;

	if (remaining > 0.0f)
	{
		ImGui::Dummy(ImVec2(0.0f, remaining));
	}

	drawRunControl();

	ImGui::Dummy(ImVec2(0.0f, 4.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextFaint);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, PDTheme::SidebarHover);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, PDTheme::SidebarHover);

	if (ImGui::Button("Quit Pony Desk", ImVec2(ImGui::GetContentRegionAvail().x, 30.0f)))
	{
		m_app.requestExit();
	}

	ImGui::PopStyleColor(4);
	ImGui::Dummy(ImVec2(0.0f, bottomGap));

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

		if (ImGui::Button("Stop", ImVec2(width, 42.0f)))
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

		if (ImGui::Button("Start", ImVec2(width, 42.0f)))
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

void PDMainWindow::beginToolbar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ToolbarPadX, ToolbarPadY));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ToolbarPadX, ToolbarPadY));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, PDTheme::Toolbar);
	ImGui::PushStyleColor(ImGuiCol_Border, PDTheme::CardBorder);

	const float height = ImGui::GetFrameHeight() + ToolbarPadY * 2.0f + 2.0f;
	constexpr ImGuiChildFlags childFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;

	ImGui::BeginChild("toolbar", ImVec2(0.0f, height), childFlags, ImGuiWindowFlags_NoScrollbar);
}

void PDMainWindow::endToolbar()
{
	ImGui::EndChild();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
	ImGui::Dummy(ImVec2(0.0f, 10.0f));
}

void PDMainWindow::toolbarSummary(const char *text)
{
	const float width = ImGui::CalcTextSize(text).x;

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - width);
	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("%s", text);
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

		bool doubleClicked = false;
		const bool clicked = drawPonyCard(
			group.displayName,
			sub,
			thumbnail(group.variants.front().previewPath),
			m_selectedGroup == static_cast<int>(index),
			doubleClicked);

		if (clicked)
		{
			m_selectedGroup = static_cast<int>(index);
		}

		if (doubleClicked)
		{
			addToScene(group.displayName, group.variants.front());
		}

		ImGui::PopID();
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();
}

bool PDMainWindow::drawPonyCard(
	std::string const &name,
	std::string const &sub,
	PDTexture const *texture,
	bool selected,
	bool &outDoubleClicked)
{
	const float nameHeight = ImGui::GetTextLineHeight();
	const float subHeight = nameHeight * 0.85f;
	const float cardHeight = CardPad + ThumbHeight + 7.0f + nameHeight + subHeight + CardPad;

	const ImVec2 position = ImGui::GetCursorScreenPos();
	const ImVec2 size(CardWidth, cardHeight);
	const ImVec2 rectMax(position.x + size.x, position.y + size.y);

	const bool clicked = ImGui::InvisibleButton("card", size);
	const bool hovered = ImGui::IsItemHovered();
	outDoubleClicked = hovered and ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

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

	return clicked;
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
		const float removeWidth = ImGui::CalcTextSize("Remove").x + 24.0f;
		const float controlsWidth = stepperWidth + 10.0f + removeWidth;

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
	if (m_editorOpen)
	{
		drawScriptEditor();

		return;
	}

	beginToolbar();

	if (ImGui::Button("Rescan"))
	{
		m_scripts.load(PONYDOCK_SCRIPTS_DIR);
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
		openScript(entry);
	}

	ImGui::SetCursorScreenPos(position);
	ImGui::Dummy(ImVec2(size.x, ScriptRowHeight + 4.0f));
}

void PDMainWindow::openScript(PDScriptEntry const &entry)
{
	std::ifstream stream(entry.fullPath, std::ios::binary);

	if (not stream)
	{
		m_diagnostics.write("Cannot open script " + entry.fullPath);

		return;
	}

	std::string const text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

	m_editor.SetLanguage(TextEditor::Language::Lua());
	m_editor.SetText(text);
	m_editingOriginal = m_editor.GetText();
	m_editingPath = entry.fullPath;
	m_editingName = entry.relativePath;
	m_editorOpen = true;
	m_editorDirty = false;
}

void PDMainWindow::saveScript()
{
	std::string const text = m_editor.GetText();
	std::ofstream stream(m_editingPath, std::ios::binary);

	if (not stream)
	{
		m_diagnostics.write("Cannot write script " + m_editingPath);

		return;
	}

	stream << text;
	stream.close();

	m_editingOriginal = text;
	m_editorDirty = false;
	m_diagnostics.write("Saved " + m_editingName);
	m_scripts.load(PONYDOCK_SCRIPTS_DIR);
	m_app.reloadScripts();
}

void PDMainWindow::drawScriptEditor()
{
	bool back = false;

	beginToolbar();

	if (ImGui::Button("Back"))
	{
		back = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("Save"))
	{
		saveScript();
	}

	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();

	if (m_editorDirty)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::AccentText);
		ImGui::Text("%s *", m_editingName.c_str());
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
		ImGui::TextUnformatted(m_editingName.c_str());
		ImGui::PopStyleColor();
	}

	endToolbar();

	if (back)
	{
		m_editorOpen = false;

		return;
	}

	const ImVec2 editorMin = ImGui::GetCursorScreenPos();
	const ImVec2 editorSize = ImGui::GetContentRegionAvail();
	const ImVec2 editorMax(editorMin.x + editorSize.x, editorMin.y + editorSize.y);

	addShadow(ImGui::GetWindowDrawList(), editorMin, editorMax);

	ImGui::PushStyleColor(ImGuiCol_Border, PDTheme::CardBorder);
	m_editor.Render("scriptEditor", editorSize, ImGuiChildFlags_Borders);
	ImGui::PopStyleColor();

	m_editorDirty = m_editor.GetText() != m_editingOriginal;
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

	if (lines.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PDTheme::TextDim);
		ImGui::TextUnformatted("Nothing logged yet.");
		ImGui::PopStyleColor();

		return;
	}

	ImGui::BeginChild("logLines");

	for (std::string const &line : lines)
	{
		ImGui::TextWrapped("%s", line.c_str());
	}

	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
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
