#include <App/PDMainApplication.h>

#include <Assets/resource.h>

#include <chrono>
#include <string>
#include <utility>

PDMainApplication::PDMainApplication()
	: m_mainWindow(*this, m_host, m_diagnostics)
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
	if (not m_overlay.initialize(instance))
	{
		m_diagnostics.write("Overlay window failed to initialize");

		return;
	}

	if (not m_spriteRenderer.initialize(m_overlay.device()))
	{
		m_diagnostics.write("Sprite renderer failed to initialize");

		return;
	}

	m_scene.initialize(m_overlay.device(), m_diagnostics, PONYDOCK_SCRIPTS_DIR);

	m_diagnostics.write("Overlay ready at " + std::to_string(m_overlay.width()) + "x" + std::to_string(m_overlay.height()));

	std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();

	while (m_overlayRunning)
	{
		m_overlay.pumpMessages();
		applyPendingScene();

		std::chrono::steady_clock::time_point const now = std::chrono::steady_clock::now();
		float const elapsedSeconds = std::chrono::duration<float>(now - lastTick).count();
		float const deltaSeconds = elapsedSeconds < MaxDeltaSeconds ? elapsedSeconds : MaxDeltaSeconds;
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
			m_scene.spawnEntity(entry.packPath, std::string(PONYDOCK_SCRIPTS_DIR) + "/core.lua", x, 0.0f);
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
