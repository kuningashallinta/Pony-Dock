#include <App/PDMainApplication.h>

#include <Assets/resource.h>

#include <chrono>
#include <string>
#include <utility>

PDMainApplication::PDMainApplication()
	: m_mainWindow(*this, m_host)
{
}

int PDMainApplication::run(HINSTANCE instance)
{
	m_host.setFrameCallback([this]()
	{
		m_mainWindow.draw();
	});

	HICON const icon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPICON));

	if (not m_host.initialize(instance, "Pony Dock", 780, 600, icon, 610, 320))
	{
		return 1;
	}

	m_overlayRunning = true;
	m_overlayThread = std::thread([this, instance]()
	{
		runOverlay(instance);
	});

	while (m_host.pumpMessages())
	{
		m_host.renderFrame();
	}

	m_overlayRunning = false;

	if (m_overlayThread.joinable())
	{
		m_overlayThread.join();
	}

	m_host.shutdown();

	return 0;
}

void PDMainApplication::runOverlay(HINSTANCE instance)
{
	if (not m_overlay.initialize(instance) or not m_spriteRenderer.initialize(m_overlay.device()))
	{
		return;
	}

	m_scene.initialize(m_overlay.device());

	std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();

	while (m_overlayRunning)
	{
		m_overlay.pumpMessages();
		applyPendingScene();

		std::chrono::steady_clock::time_point const now = std::chrono::steady_clock::now();
		float const deltaSeconds = std::chrono::duration<float>(now - lastTick).count();
		lastTick = now;

		m_scene.tick(deltaSeconds, m_overlay.width(), m_overlay.height());

		m_overlay.beginFrame();
		m_spriteRenderer.begin(m_overlay.context(), m_overlay.width(), m_overlay.height());
		m_scene.draw(m_spriteRenderer);
		m_overlay.endFrame();
	}

	m_spriteRenderer.shutdown();
	m_overlay.shutdown();
}

void PDMainApplication::applyPendingScene()
{
	std::vector<PDSceneEntry> entries;

	{
		std::lock_guard<std::mutex> const lock(m_sceneMutex);

		if (not m_pendingApply)
		{
			return;
		}

		entries = std::move(m_pendingEntries);
		m_pendingEntries.clear();
		m_pendingApply = false;
	}

	m_scene.clear();

	float x = 0.0f;

	for (PDSceneEntry const &entry : entries)
	{
		for (int i = 0; i < entry.quantity; i += 1)
		{
			m_scene.spawnEntity(entry.previewPath, std::string(PONYDOCK_SCRIPTS_DIR) + "/walk.lua", x, 0.0f);
			x += 120.0f;
		}
	}
}

void PDMainApplication::startScene(std::vector<PDSceneEntry> const &entries)
{
	{
		std::lock_guard<std::mutex> const lock(m_sceneMutex);
		m_pendingEntries = entries;
		m_pendingApply = true;
	}

	m_sceneRunning = true;
}

void PDMainApplication::stopScene()
{
	{
		std::lock_guard<std::mutex> const lock(m_sceneMutex);
		m_pendingEntries.clear();
		m_pendingApply = true;
	}

	m_sceneRunning = false;
}

void PDMainApplication::requestExit()
{
	m_host.close();
}
