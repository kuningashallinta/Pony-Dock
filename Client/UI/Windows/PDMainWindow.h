#pragma once

#include <Engine/PDDiagnostics.h>
#include <Engine/PDTexture.h>
#include <Library/PDPonyCatalog.h>
#include <Library/PDSceneEntry.h>

#include <imgui.h>

#include <string>
#include <unordered_map>
#include <vector>

class PDMainApplication;
class PDImGui;

class PDMainWindow
{
public:
	PDMainWindow(PDMainApplication &app, PDImGui &host, PDDiagnostics &diagnostics);

	void draw();

private:
	static constexpr float NavWidth = 224.0f;
	static constexpr float CardWidth = 150.0f;
	static constexpr float ThumbHeight = 108.0f;
	static constexpr float CardPad = 8.0f;
	static constexpr float CardSpacing = 14.0f;

	enum class View
	{
		Browser,
		Scene,
		Modules,
		Settings,
		Log
	};

	void drawSidebar(float width, float height);
	void navItem(const char *label, View view);
	void drawRunControl();
	void setView(View view);

	void drawContent();
	void drawBrowserView();
	bool drawPonyCard(std::string const &name, std::string const &sub, PDTexture const *texture, bool selected, bool &outDoubleClicked);
	PDTexture *thumbnail(std::string const &path);

	void drawSceneView();
	void addToScene(std::string const &displayName, PDPonyPack const &pack);
	void removeSceneEntry(std::size_t index);
	int sceneTotalQuantity() const;

	void drawLogView();

	static std::string toLower(std::string text);
	static void addImageFitted(ImDrawList *drawList, PDTexture const *texture, ImVec2 areaMin, ImVec2 areaMax, float margin);

	PDMainApplication &m_app;
	PDImGui &m_host;
	PDDiagnostics &m_diagnostics;
	View m_activeView = View::Browser;

	PDPonyCatalog m_catalog;
	std::unordered_map<std::string, PDTexture> m_thumbnails;
	int m_thumbnailBudget = 0;

	char m_search[128] = "";
	int m_selectedGroup = -1;

	std::vector<PDSceneEntry> m_scene;
};
