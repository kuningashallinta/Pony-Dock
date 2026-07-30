#pragma once

#include <Library/PDPonyCatalog.h>
#include <UI/PDTexture.h>

#include <string>
#include <unordered_map>

class PDMainApplication;
class PDImGui;

class PDMainWindow
{
public:
	PDMainWindow(PDMainApplication &app, PDImGui &host);

	void draw();

private:
	enum class View
	{
		Browser,
		Scene,
		Modules,
		Settings
	};

	void drawSidebar(float width, float height);
	void navItem(const char *label, View view);
	void drawRunControl();
	void setView(View view);

	void drawContent();
	void drawBrowserView();
	bool drawPonyCard(std::string const &name, std::string const &sub, PDTexture const *texture, bool selected);
	PDTexture *thumbnail(std::string const &path);

	PDMainApplication &m_app;
	PDImGui &m_host;
	View m_activeView = View::Browser;

	PDPonyCatalog m_catalog;
	std::unordered_map<std::string, PDTexture> m_thumbnails;
	int m_thumbnailBudget = 0;

	char m_search[128] = "";
	int m_selectedGroup = -1;
};
