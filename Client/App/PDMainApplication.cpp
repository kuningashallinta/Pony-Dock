#include <App/PDMainApplication.h>

PDMainApplication::PDMainApplication()
	: m_mainWindow(*this)
{
}

int PDMainApplication::run(HINSTANCE instance)
{
	m_host.setFrameCallback([this]() { m_mainWindow.draw(); });

	if (not m_host.initialize(instance, "Pony Dock", 720, 480))
	{
		return 1;
	}

	while (m_host.pumpMessages())
	{
		m_host.renderFrame();
	}

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
