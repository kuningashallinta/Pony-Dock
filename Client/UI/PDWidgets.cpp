#include <UI/PDWidgets.h>

#include <UI/PDTheme.h>

static constexpr float ToolbarPadX = 10.0f;
static constexpr float ToolbarPadY = 6.0f;
static constexpr float ModalPad = 16.0f;

void beginToolbar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ToolbarPadX, ToolbarPadY));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ToolbarPadX, ToolbarPadY));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, PDTheme::Toolbar);
	ImGui::PushStyleColor(ImGuiCol_Border, PDTheme::CardBorder);

	const float height = ImGui::GetFrameHeight() + ToolbarPadY * 2.0f + 2.0f;
	ImGui::BeginChild(
		"toolbar",
		ImVec2(0.0f, height),
		ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
		ImGuiWindowFlags_NoScrollbar);
}

void endToolbar()
{
	ImGui::EndChild();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
	ImGui::Dummy(ImVec2(0.0f, 10.0f));
}

void toolbarSummary(const char *text)
{
	const float width = ImGui::CalcTextSize(text).x;

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - width);
	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("%s", text);
}

bool beginModal(const char *title, ImVec2 size)
{
	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(size, ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, PDTheme::Popup);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ModalPad, ModalPad));

	const bool open = ImGui::BeginPopupModal(
		title,
		nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

	ImGui::PopStyleVar();

	if (not open)
	{
		ImGui::PopStyleColor();
	}

	return open;
}

void endModal()
{
	ImGui::EndPopup();
	ImGui::PopStyleColor();
}

PDRowCard beginRowCard(float height, bool selected)
{
	PDRowCard card;
	card.position = ImGui::GetCursorScreenPos();
	card.width = ImGui::GetContentRegionAvail().x;
	card.rectMax = ImVec2(card.position.x + card.width, card.position.y + height);

	ImGui::SetNextItemAllowOverlap();
	card.clicked = ImGui::InvisibleButton("row", ImVec2(card.width, height));
	card.hovered = ImGui::IsItemHovered();

	ImVec4 const &background = card.hovered ? PDTheme::CardBgHover : PDTheme::CardBg;
	ImVec4 const &border = selected ? PDTheme::Accent : PDTheme::CardBorder;

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(card.position, card.rectMax, ImGui::GetColorU32(background), 0.0f);
	drawList->AddRect(card.position, card.rectMax, ImGui::GetColorU32(border), 0.0f, 0, selected ? 2.0f : 1.0f);

	return card;
}

void endRowCard(PDRowCard const &card, float height, float gap)
{
	ImGui::SetCursorScreenPos(card.position);
	ImGui::Dummy(ImVec2(card.width, height + gap));
}
