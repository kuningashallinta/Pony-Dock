#pragma once

class PDMainApplication;

class PDMainWindow
{
public:
	explicit PDMainWindow(PDMainApplication &app);

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

	PDMainApplication &m_app;
	View m_activeView = View::Browser;
};
