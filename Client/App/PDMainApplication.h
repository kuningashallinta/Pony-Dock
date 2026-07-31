#pragma once

#include <Engine/PDDiagnostics.h>
#include <Engine/PDOverlayWindow.h>
#include <Engine/PDScene.h>
#include <Engine/PDSpriteRenderer.h>
#include <Library/PDSceneEntry.h>
#include <UI/PDImGui.h>
#include <UI/Windows/PDMainWindow.h>

#include <windows.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class PDMainApplication
{
public:
	PDMainApplication();

	int run(HINSTANCE instance);

	bool isRunning() const
	{
		return m_sceneRunning;
	}

	void startScene(std::vector<PDSceneEntry> const &entries);
	void stopScene();
	void reloadScripts();
	void requestExit();

	void setScriptLoaded(std::string const &scriptPath, bool loaded);
	std::vector<std::string> loadedScripts() const;

	static std::string coreScriptPath();
	static std::vector<std::string> requiredScripts();

private:
	static constexpr float MaxDeltaSeconds = 0.1f;

	struct ScriptCommand
	{
		std::string path;
		bool load = false;
	};

	void runOverlay(HINSTANCE instance);
	void applyPendingScene();
	void applyPendingScripts();

	PDDiagnostics m_diagnostics;
	PDImGui m_host;
	PDMainWindow m_mainWindow;

	PDOverlayWindow m_overlay;
	PDSpriteRenderer m_spriteRenderer;
	PDScene m_scene;

	std::thread m_overlayThread;
	std::atomic<bool> m_overlayRunning = false;
	std::atomic<bool> m_sceneRunning = false;

	std::atomic<bool> m_pendingReload = false;

	std::mutex m_sceneMutex;
	std::vector<PDSceneEntry> m_pendingEntries;
	bool m_pendingApply = false;

	mutable std::mutex m_scriptsMutex;
	std::vector<std::string> m_loadedScripts;
	std::vector<ScriptCommand> m_pendingScripts;
};
