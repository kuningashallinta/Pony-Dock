#include <UI/PDTheme.h>

void PDTheme::apply()
{
	ImGuiStyle &style = ImGui::GetStyle();

	style.Colors[ImGuiCol_FrameBg] = InputBg;
	style.Colors[ImGuiCol_FrameBgHovered] = InputBgHover;
	style.Colors[ImGuiCol_FrameBgActive] = InputBgActive;
	style.Colors[ImGuiCol_Border] = CardBorder;

	style.Colors[ImGuiCol_CheckMark] = Accent;
	style.Colors[ImGuiCol_SliderGrab] = Accent;
	style.Colors[ImGuiCol_SliderGrabActive] = AccentText;

	style.Colors[ImGuiCol_Button] = SidebarHover;
	style.Colors[ImGuiCol_ButtonHovered] = CardBgHover;
	style.Colors[ImGuiCol_ButtonActive] = CardBg;
}
