#pragma once

#include <Engine/PDDiagnostics.h>
#include <Engine/PDTexture.h>
#include <Library/PDPonyCatalog.h>
#include <Library/PDSceneEntry.h>
#include <Library/PDScriptCatalog.h>
#include <Library/PDSettings.h>

#include <imgui.h>

#include <string>
#include <unordered_map>
#include <vector>

class PDMainApplication;
class PDImGui;

struct PDSettingTarget
{
	std::string id;
	std::string label;
	std::string previewPath;
};

class PDMainWindow
{
public:
	PDMainWindow(PDMainApplication &app, PDImGui &host, PDDiagnostics &diagnostics);

	void draw();

private:
	static constexpr float NavWidth = 224.0f;
	static constexpr float SidebarPad = 12.0f;
	static constexpr float RunButtonHeight = 42.0f;
	static constexpr float QuitButtonHeight = 30.0f;
	static constexpr float FooterGap = 4.0f;
	static constexpr float CardWidth = 150.0f;
	static constexpr float ThumbHeight = 108.0f;
	static constexpr float CardPad = 8.0f;
	static constexpr float CardSpacing = 14.0f;
	static constexpr float ScriptRowHeight = 34.0f;
	static constexpr float ColumnGap = 18.0f;
	static constexpr float RowPad = 10.0f;
	static constexpr float ToolbarPadX = 10.0f;
	static constexpr float ToolbarPadY = 6.0f;
	static constexpr float ToggleWidth = 78.0f;
	static constexpr float ToggleHeight = 22.0f;
	static constexpr float ShadowSpread = 6.0f;
	static constexpr float SettingRowHeight = 48.0f;
	static constexpr float SettingControlWidth = 190.0f;
	static constexpr float TargetPickerWidth = 170.0f;
	static constexpr float ModalPad = 16.0f;

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

	void beginToolbar();
	void endToolbar();
	void toolbarSummary(const char *text);

	void drawContent();
	void drawBrowserView();
	bool drawPonyCard(std::string const &name, std::string const &sub, PDTexture const *texture, bool selected, bool &outDoubleClicked);
	PDTexture *thumbnail(std::string const &path);

	void drawSceneView();
	void addToScene(std::string const &displayName, PDPonyPack const &pack);
	void removeSceneEntry(std::size_t index);
	int sceneTotalQuantity() const;

	void drawLogView();

	void drawModulesView();
	void drawScriptColumn(const char *label, bool loadedColumn, float width);
	void drawColumnHeader(const char *label, int count);
	void drawScriptCard(PDScriptEntry const &entry, bool loaded);
	void openModule(PDScriptEntry const &entry);

	void drawModuleSettings();
	void drawSettingRow(PDSettingDeclaration const &declaration, bool loaded);
	void drawTargetButton();
	void drawTargetModal();
	void openInEditor();

	static void addImageFitted(ImDrawList *drawList, PDTexture const *texture, ImVec2 areaMin, ImVec2 areaMax, float margin);
	static void addShadow(ImDrawList *drawList, ImVec2 rectMin, ImVec2 rectMax);

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

	PDScriptCatalog m_scripts;
	std::vector<std::string> m_loadedScripts;
	std::vector<std::string> m_requiredScripts;
	std::string m_settingsModule;
	std::string m_settingsModulePath;
	std::string m_settingsTarget;
	std::string m_settingsTargetLabel;
	char m_targetFilter[64] = "";
	bool m_targetPickerOpen = false;

	std::vector<PDSettingTarget> m_targets;
};
