#include <UI/PDTheme.h>

void PDTheme::apply()
{
	ImGuiStyle &style = ImGui::GetStyle();

	style.Colors[ImGuiCol_WindowBg] = Window;
	style.Colors[ImGuiCol_ChildBg] = Clear;
	style.Colors[ImGuiCol_FrameBg] = InputBg;
	style.Colors[ImGuiCol_FrameBgHovered] = InputBgHover;
	style.Colors[ImGuiCol_FrameBgActive] = InputBgActive;
	style.Colors[ImGuiCol_Border] = CardBorder;

	style.Colors[ImGuiCol_CheckMark] = White;
	style.Colors[ImGuiCol_CheckboxSelectedBg] = Accent;
	style.Colors[ImGuiCol_SliderGrab] = Accent;
	style.Colors[ImGuiCol_SliderGrabActive] = AccentText;

	style.Colors[ImGuiCol_Button] = SidebarHover;
	style.Colors[ImGuiCol_ButtonHovered] = CardBgHover;
	style.Colors[ImGuiCol_ButtonActive] = CardBg;

	style.Colors[ImGuiCol_PopupBg] = Popup;
	style.Colors[ImGuiCol_ModalWindowDimBg] = Dim;
	style.Colors[ImGuiCol_TitleBg] = Sidebar;
	style.Colors[ImGuiCol_TitleBgActive] = Sidebar;
	style.Colors[ImGuiCol_TitleBgCollapsed] = Sidebar;

	style.Colors[ImGuiCol_Header] = AccentSoft;
	style.Colors[ImGuiCol_HeaderHovered] = AccentSoftHover;
	style.Colors[ImGuiCol_HeaderActive] = Accent;

	style.Colors[ImGuiCol_Separator] = Divider;
	style.Colors[ImGuiCol_SeparatorHovered] = Accent;
	style.Colors[ImGuiCol_SeparatorActive] = Accent;

	style.Colors[ImGuiCol_ResizeGrip] = Clear;
	style.Colors[ImGuiCol_ResizeGripHovered] = AccentSoft;
	style.Colors[ImGuiCol_ResizeGripActive] = Accent;

	style.Colors[ImGuiCol_Tab] = CardBg;
	style.Colors[ImGuiCol_TabHovered] = AccentSoftHover;
	style.Colors[ImGuiCol_TabSelected] = AccentSoft;
	style.Colors[ImGuiCol_TabSelectedOverline] = Accent;
	style.Colors[ImGuiCol_TabDimmed] = CardBg;
	style.Colors[ImGuiCol_TabDimmedSelected] = AccentSoft;
	style.Colors[ImGuiCol_TabDimmedSelectedOverline] = Accent;

	style.Colors[ImGuiCol_ScrollbarBg] = Clear;
	style.Colors[ImGuiCol_ScrollbarGrab] = Scroll;
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ScrollHover;
	style.Colors[ImGuiCol_ScrollbarGrabActive] = Accent;

	style.Colors[ImGuiCol_TextSelectedBg] = AccentSoftHover;
	style.Colors[ImGuiCol_NavCursor] = Accent;
	style.Colors[ImGuiCol_NavWindowingHighlight] = Accent;
	style.Colors[ImGuiCol_DragDropTarget] = Accent;
	style.Colors[ImGuiCol_DragDropTargetBg] = AccentSoft;

	style.Colors[ImGuiCol_PlotLines] = TextDim;
	style.Colors[ImGuiCol_PlotLinesHovered] = AccentText;
	style.Colors[ImGuiCol_PlotHistogram] = Accent;
	style.Colors[ImGuiCol_PlotHistogramHovered] = AccentText;

	style.Colors[ImGuiCol_TableHeaderBg] = CardBg;
	style.Colors[ImGuiCol_TableBorderStrong] = CardBorder;
	style.Colors[ImGuiCol_TableBorderLight] = Divider;
	style.Colors[ImGuiCol_TableRowBg] = Clear;
	style.Colors[ImGuiCol_TableRowBgAlt] = CardBg;

	style.Colors[ImGuiCol_Text] = White;
	style.Colors[ImGuiCol_TextDisabled] = TextFaint;
	style.Colors[ImGuiCol_TextLink] = AccentText;
	style.Colors[ImGuiCol_InputTextCursor] = White;
	style.Colors[ImGuiCol_TreeLines] = Divider;
	style.Colors[ImGuiCol_UnsavedMarker] = AccentText;
	style.Colors[ImGuiCol_MenuBarBg] = Sidebar;
	style.Colors[ImGuiCol_BorderShadow] = Clear;
	style.Colors[ImGuiCol_NavWindowingDimBg] = Dim;
}
