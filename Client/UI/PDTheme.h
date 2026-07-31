#pragma once

#include <imgui.h>

namespace PDTheme
{
	inline ImVec4 const Accent = ImVec4(0.6509804f, 0.14901961f, 0.34509805f, 1.0f);
	inline ImVec4 const AccentSoft = ImVec4(0.24705882f, 0.1254902f, 0.18823529f, 1.0f);
	inline ImVec4 const AccentText = ImVec4(0.92156863f, 0.61960787f, 0.7372549f, 1.0f);
	inline ImVec4 const AccentHover = ImVec4(0.7254902f, 0.19607843f, 0.40392157f, 1.0f);
	inline ImVec4 const AccentPress = ImVec4(0.54901963f, 0.11372549f, 0.28627452f, 1.0f);

	inline ImVec4 const Start = ImVec4(0.18f, 0.52f, 0.27f, 1.0f);
	inline ImVec4 const StartHover = ImVec4(0.22f, 0.6f, 0.31f, 1.0f);
	inline ImVec4 const StartPress = ImVec4(0.14f, 0.44f, 0.22f, 1.0f);
	inline ImVec4 const Stop = ImVec4(0.6f, 0.16f, 0.16f, 1.0f);
	inline ImVec4 const StopHover = ImVec4(0.7f, 0.22f, 0.22f, 1.0f);
	inline ImVec4 const StopPress = ImVec4(0.5f, 0.12f, 0.12f, 1.0f);

	inline ImVec4 const Sidebar = ImVec4(0.07f, 0.07f, 0.07f, 1.0f);
	inline ImVec4 const SidebarHover = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
	inline ImVec4 const Divider = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
	inline ImVec4 const TextDim = ImVec4(0.62f, 0.62f, 0.62f, 1.0f);
	inline ImVec4 const TextFaint = ImVec4(0.46f, 0.46f, 0.46f, 1.0f);
	inline ImVec4 const White = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);

	inline ImVec4 const CardBg = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	inline ImVec4 const CardBgHover = ImVec4(0.17f, 0.17f, 0.17f, 1.0f);
	inline ImVec4 const CardBorder = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
	inline ImVec4 const ThumbBg = ImVec4(0.09f, 0.09f, 0.09f, 1.0f);
	inline ImVec4 const Toolbar = ImVec4(0.105f, 0.105f, 0.105f, 1.0f);
	inline ImVec4 const Badge = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
	inline ImVec4 const Shadow = ImVec4(0.0f, 0.0f, 0.0f, 0.34f);

	inline ImVec4 const InputBg = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	inline ImVec4 const InputBgHover = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
	inline ImVec4 const InputBgActive = ImVec4(0.17f, 0.17f, 0.17f, 1.0f);

	// Applies the palette above to ImGui's global style so every stock widget
	// (input boxes, checkboxes, sliders) matches instead of ImGui's default blue.
	void apply();
} // namespace PDTheme
