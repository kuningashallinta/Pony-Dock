#include <App/PDMainApplication.h>

#include <Assets/resource.h>

#include <chrono>
#include <filesystem>
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

	m_settings.load();
	m_diagnostics.write("Settings at " + m_settings.path());

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

	m_settings.save();
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

	m_scene.initialize(m_overlay.device(), m_diagnostics, m_settings, PONYDOCK_SCRIPTS_DIR);
	m_scene.loadScript(coreScriptPath());

	m_diagnostics.write("Overlay ready at " + std::to_string(m_overlay.width()) + "x" + std::to_string(m_overlay.height()));

	std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();

	while (m_overlayRunning)
	{
		m_overlay.pumpMessages();

		if (m_pendingReload.exchange(false))
		{
			m_scene.reloadScripts();
			m_scene.loadScript(coreScriptPath());
		}

		applyPendingScripts();
		applyPendingScene();

		std::chrono::steady_clock::time_point const now = std::chrono::steady_clock::now();
		float const elapsedSeconds = std::chrono::duration<float>(now - lastTick).count();
		float const deltaSeconds = elapsedSeconds < MaxDeltaSeconds ? elapsedSeconds : MaxDeltaSeconds;
		lastTick = now;

		m_scene.tick(deltaSeconds, m_overlay.width(), m_overlay.height());
		applyPendingButtons();

		{
			std::lock_guard<std::mutex> const lock(m_scriptsMutex);
			m_loadedScripts = m_scene.loadedScripts();
		}

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
			m_scene.spawnEntity(entry.packPath, coreScriptPath(), x, 0.0f);
			x += 120.0f;
		}
	}
}

void PDMainApplication::applyPendingScripts()
{
	std::vector<ScriptCommand> commands;

	{
		std::lock_guard<std::mutex> const lock(m_scriptsMutex);

		if (m_pendingScripts.empty())
		{
			return;
		}

		commands = std::move(m_pendingScripts);
		m_pendingScripts.clear();
	}

	for (ScriptCommand const &command : commands)
	{
		if (command.load)
		{
			m_scene.loadScript(command.path);
		}
		else
		{
			m_scene.unloadScript(command.path);
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

void PDMainApplication::reloadScripts()
{
	m_pendingReload = true;
}

void PDMainApplication::applyPendingButtons()
{
	std::vector<ButtonPress> presses;

	{
		std::lock_guard<std::mutex> const lock(m_buttonsMutex);

		if (m_pendingButtons.empty())
		{
			return;
		}

		presses = std::move(m_pendingButtons);
		m_pendingButtons.clear();
	}

	for (ButtonPress const &press : presses)
	{
		m_scene.pressSettingsButton(press.moduleKey, press.settingId);
	}
}

void PDMainApplication::pressSettingButton(std::string const &moduleKey, std::string const &settingId)
{
	std::lock_guard<std::mutex> const lock(m_buttonsMutex);

	if (m_pendingButtons.size() >= MaxPendingButtons)
	{
		return;
	}

	ButtonPress press;
	press.moduleKey = moduleKey;
	press.settingId = settingId;
	m_pendingButtons.push_back(std::move(press));
}

void PDMainApplication::setScriptLoaded(std::string const &scriptPath, bool loaded)
{
	ScriptCommand command;
	command.path = scriptPath;
	command.load = loaded;

	std::lock_guard<std::mutex> const lock(m_scriptsMutex);
	m_pendingScripts.push_back(std::move(command));
}

std::vector<std::string> PDMainApplication::loadedScripts() const
{
	std::lock_guard<std::mutex> const lock(m_scriptsMutex);

	return m_loadedScripts;
}

std::string PDMainApplication::coreScriptPath()
{
	return std::filesystem::path(std::string(PONYDOCK_SCRIPTS_DIR) + "/core.lua").lexically_normal().string();
}

std::string PDMainApplication::scriptsRoot()
{
	return PONYDOCK_SCRIPTS_DIR;
}

std::vector<std::string> PDMainApplication::requiredScripts()
{
	std::string const library = std::filesystem::path(std::string(PONYDOCK_SCRIPTS_DIR) + "/lib/pd.lua").lexically_normal().string();

	return {coreScriptPath(), library};
}

void PDMainApplication::requestExit()
{
	m_host.close();
}
