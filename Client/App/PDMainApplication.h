#pragma once

#include <Engine/PDOverlayWindow.h>
#include <Engine/PDScene.h>
#include <Engine/PDSpriteRenderer.h>
#include <UI/PDImGui.h>
#include <UI/Windows/PDMainWindow.h>

#include <windows.h>

class PDMainApplication
{
public:
	PDMainApplication();

	int run(HINSTANCE instance);

	bool isRunning() const
	{
		return m_sceneRunning;
	}

	void startScene();
	void stopScene();
	void requestExit();

private:
	PDImGui m_host;
	PDMainWindow m_mainWindow;

	PDOverlayWindow m_overlay;
	PDSpriteRenderer m_spriteRenderer;
	PDScene m_scene;

	bool m_sceneRunning = false;
};
