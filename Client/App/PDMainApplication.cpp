#include <App/PDMainApplication.h>

#include <Assets/resource.h>

#include <chrono>

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

	if (not m_overlay.initialize(instance) or not m_spriteRenderer.initialize(m_overlay.device()))
	{
		return 1;
	}

	m_scene.initialize(m_overlay.device());
	m_scene.spawnEntity(std::string(PONYDOCK_PACKS_DIR) + "/fluttershy/preview.png", 0.0f, 0.0f);

	std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();

	while (m_host.pumpMessages())
	{
		std::chrono::steady_clock::time_point const now = std::chrono::steady_clock::now();
		float const deltaSeconds = std::chrono::duration<float>(now - lastTick).count();
		lastTick = now;

		m_host.renderFrame();

		m_scene.tick(deltaSeconds, m_overlay.width(), m_overlay.height());
		m_overlay.beginFrame();
		m_spriteRenderer.begin(m_overlay.context(), m_overlay.width(), m_overlay.height());
		m_scene.draw(m_spriteRenderer);
		m_overlay.endFrame();
	}

	m_spriteRenderer.shutdown();
	m_overlay.shutdown();
	m_host.shutdown();

	return 0;
}

void PDMainApplication::startScene()
{
	m_sceneRunning = true;
}

void PDMainApplication::stopScene()
{
	m_sceneRunning = false;
}

void PDMainApplication::requestExit()
{
	m_host.close();
}
