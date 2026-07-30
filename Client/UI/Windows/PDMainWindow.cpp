#include <UI/Windows/PDMainWindow.h>

#include <App/PDMainApplication.h>

#include <imgui.h>

namespace
{
	const ImVec4 kAccent = ImVec4(0.6509804f, 0.14901961f, 0.34509805f, 1.0f);
	const ImVec4 kAccentSoft = ImVec4(0.24705882f, 0.1254902f, 0.18823529f, 1.0f);
	const ImVec4 kAccentText = ImVec4(0.92156863f, 0.61960787f, 0.7372549f, 1.0f);

	const ImVec4 kStart = ImVec4(0.18f, 0.52f, 0.27f, 1.0f);
	const ImVec4 kStartHover = ImVec4(0.22f, 0.6f, 0.31f, 1.0f);
	const ImVec4 kStartPress = ImVec4(0.14f, 0.44f, 0.22f, 1.0f);
	const ImVec4 kStop = ImVec4(0.6f, 0.16f, 0.16f, 1.0f);
	const ImVec4 kStopHover = ImVec4(0.7f, 0.22f, 0.22f, 1.0f);
	const ImVec4 kStopPress = ImVec4(0.5f, 0.12f, 0.12f, 1.0f);

	const ImVec4 kSidebar = ImVec4(0.07f, 0.07f, 0.07f, 1.0f);
	const ImVec4 kSidebarHover = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
	const ImVec4 kDivider = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
	const ImVec4 kTextDim = ImVec4(0.62f, 0.62f, 0.62f, 1.0f);
	const ImVec4 kTextFaint = ImVec4(0.46f, 0.46f, 0.46f, 1.0f);
	const ImVec4 kWhite = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);

	constexpr float NavWidth = 224.0f;

	ImU32 u32(const ImVec4 &color)
	{
		return ImGui::GetColorU32(color);
	}
} // namespace

PDMainWindow::PDMainWindow(PDMainApplication &app)
	: m_app(app)
{
}

void PDMainWindow::draw()
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Pony Dock", nullptr, flags);
	ImGui::PopStyleVar(3);

	const float height = ImGui::GetContentRegionAvail().y;
	drawSidebar(NavWidth, height);

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const ImVec2 windowPos = ImGui::GetWindowPos();
	drawList->AddLine(ImVec2(windowPos.x + NavWidth, windowPos.y), ImVec2(windowPos.x + NavWidth, windowPos.y + height),
		u32(kDivider), 1.0f);

	ImGui::End();
}

void PDMainWindow::drawSidebar(float width, float height)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, kSidebar);
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
	ImGui::PushStyleColor(ImGuiCol_Text, kTextFaint);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kSidebarHover);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, kSidebarHover);

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
		drawList->AddRectFilled(position, rectMax, u32(kAccentSoft), 0.0f);
		drawList->AddRectFilled(position, ImVec2(position.x + 3.0f, rectMax.y), u32(kAccent), 0.0f);
	}
	else if (hovered)
	{
		drawList->AddRectFilled(position, rectMax, u32(kSidebarHover), 0.0f);
	}

	const ImVec4 &textColor = active ? kAccentText : (hovered ? kWhite : kTextDim);
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
		ImGui::PushStyleColor(ImGuiCol_Button, kStop);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kStopHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, kStopPress);

		if (ImGui::Button("Stop", ImVec2(width, 42.0f)))
		{
			m_app.stopScene();
		}
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, kStart);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kStartHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, kStartPress);

		if (ImGui::Button("Start", ImVec2(width, 42.0f)))
		{
			m_app.startScene();
		}
	}

	ImGui::PopStyleColor(3);
}

void PDMainWindow::setView(View view)
{
	m_activeView = view;
}
