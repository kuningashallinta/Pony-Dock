#pragma once

#include <Engine/PDDiagnostics.h>
#include <Engine/PDTexture.h>
#include <Library/PDPonyCatalog.h>
#include <Library/PDSceneEntry.h>

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
