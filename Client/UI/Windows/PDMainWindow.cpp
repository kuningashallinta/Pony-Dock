#include <UI/Windows/PDMainWindow.h>

#include <App/PDMainApplication.h>
#include <UI/PDImGui.h>
#include <UI/PDTheme.h>

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <utility>

namespace
{
	constexpr float NavWidth = 224.0f;
	constexpr float CardWidth = 150.0f;
	constexpr float ThumbHeight = 108.0f;
	constexpr float CardPad = 8.0f;
	constexpr float CardSpacing = 14.0f;

	ImU32 u32(const ImVec4 &color)
	{
		return ImGui::GetColorU32(color);
	}

	std::string toLower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
		{
			return static_cast<char>(std::tolower(c));
		});

		return text;
	}

	void addImageFitted(ImDrawList *drawList, PDTexture const *texture, ImVec2 areaMin, ImVec2 areaMax, float margin)
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
} // namespace

PDMainWindow::PDMainWindow(PDMainApplication &app, PDImGui &host)
	: m_app(app),
	  m_host(host)
{
	m_catalog.load(PONYDOCK_PACKS_DIR);
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
		u32(PDTheme::Divider),
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
		drawList->AddRectFilled(position, rectMax, u32(PDTheme::AccentSoft), 0.0f);
		drawList->AddRectFilled(position, ImVec2(position.x + 3.0f, rectMax.y), u32(PDTheme::Accent), 0.0f);
	}
	else if (hovered)
	{
		drawList->AddRectFilled(position, rectMax, u32(PDTheme::SidebarHover), 0.0f);
	}

	const ImVec4 &textColor = active ? PDTheme::AccentText : (hovered ? PDTheme::White : PDTheme::TextDim);
	const float textY = position.y + (itemHeight - ImGui::GetTextLineHeight()) * 0.5f;
	drawList->AddText(ImVec2(position.x + 16.0f, textY), u32(textColor), label);

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

		default:
		{
			break;
		}
	}
}

void PDMainWindow::drawBrowserView()
{
	m_thumbnailBudget = 24;

	const float searchWidth = 250.0f;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - searchWidth);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
	ImGui::SetNextItemWidth(searchWidth);
	ImGui::InputTextWithHint("##search", "Search ponies", m_search, sizeof(m_search));
	ImGui::PopStyleVar();

	ImGui::Dummy(ImVec2(0.0f, 8.0f));
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
	drawList->AddRectFilled(position, rectMax, u32(hovered ? PDTheme::CardBgHover : PDTheme::CardBg), 0.0f);

	const ImVec2 thumbMin(position.x + CardPad, position.y + CardPad);
	const ImVec2 thumbMax(rectMax.x - CardPad, position.y + CardPad + ThumbHeight);
	drawList->AddRectFilled(thumbMin, thumbMax, u32(PDTheme::ThumbBg), 0.0f);
	drawList->AddRect(thumbMin, thumbMax, u32(PDTheme::CardBorder), 0.0f, 0, 1.0f);
	addImageFitted(drawList, texture, thumbMin, thumbMax, 6.0f);

	drawList->AddText(ImVec2(position.x + CardPad + 2.0f, thumbMax.y + 7.0f), u32(PDTheme::White), name.c_str());

	if (not sub.empty())
	{
		drawList->AddText(ImVec2(position.x + CardPad + 2.0f, thumbMax.y + 7.0f + nameHeight + 2.0f), u32(PDTheme::TextFaint), sub.c_str());
	}

	drawList->AddRect(position, rectMax, u32(selected ? PDTheme::Accent : PDTheme::CardBorder), 0.0f, 0, selected ? 2.0f : 1.0f);

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
	if (ImGui::Button("Clear scene"))
	{
		m_scene.clear();
	}

	ImGui::SameLine();

	char summary[64];
	std::snprintf(summary, sizeof(summary), "%d across %zu packs", sceneTotalQuantity(), m_scene.size());
	const float summaryWidth = ImGui::CalcTextSize(summary).x;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - summaryWidth);
	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("%s", summary);

	ImGui::Dummy(ImVec2(0.0f, 8.0f));

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
		drawList->AddRectFilled(rowMin, rowMax, u32(PDTheme::CardBg), 0.0f);
		drawList->AddRect(rowMin, rowMax, u32(PDTheme::CardBorder), 0.0f, 0, 1.0f);

		const ImVec2 thumbMin(rowMin.x + 8.0f, rowMin.y + 8.0f);
		const ImVec2 thumbMax(thumbMin.x + 40.0f, thumbMin.y + 40.0f);
		drawList->AddRectFilled(thumbMin, thumbMax, u32(PDTheme::ThumbBg), 0.0f);
		drawList->AddRect(thumbMin, thumbMax, u32(PDTheme::CardBorder), 0.0f, 0, 1.0f);
		addImageFitted(drawList, thumbnail(entry.previewPath), thumbMin, thumbMax, 3.0f);

		drawList->AddText(ImVec2(thumbMax.x + 12.0f, rowMin.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f), u32(PDTheme::White), entry.displayName.c_str());

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
		drawList->AddText(ImVec2(quantityPos.x + (32.0f - quantityWidth) * 0.5f, quantityPos.y + 4.0f), u32(PDTheme::White), quantity);
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

int PDMainWindow::sceneTotalQuantity() const
{
	int total = 0;

	for (PDSceneEntry const &entry : m_scene)
	{
		total += entry.quantity;
	}

	return total;
}
