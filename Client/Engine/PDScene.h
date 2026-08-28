#pragma once

#include <Engine/Components/PDAnimation.h>
#include <Engine/Components/PDLabel.h>
#include <Engine/Components/PDPack.h>
#include <Engine/Components/PDPosition.h>
#include <Engine/Components/PDScript.h>
#include <Engine/Components/PDSprite.h>
#include <Engine/PDAnimationCache.h>
#include <Engine/PDDiagnostics.h>
#include <Engine/PDLua.h>
#include <Engine/PDSpriteRenderer.h>
#include <Engine/PDTextCache.h>
#include <Engine/PDTextureCache.h>
#include <Library/PDPonyPackData.h>

#include <d3d11.h>

#include <entt/entt.hpp>

#include <string>
#include <unordered_map>

class PDScene
{
public:
	~PDScene();

	void initialize(
		ID3D11Device *device,
		PDDiagnostics &diagnostics,
		PDSettingsStore &settings,
		std::string const &scriptsRoot);

	void spawnEntity(std::string const &packPath, std::string const &scriptPath, float x, float y);
	void clear();
	void reloadScripts();
	void reloadPack(std::string const &packPath);

	std::vector<std::string> loadedScripts() const
	{
		return m_lua.loadedScripts();
	}

	bool loadScript(std::string const &scriptPath)
	{
		return m_lua.loadScript(scriptPath);
	}

	void unloadScript(std::string const &scriptPath)
	{
		m_lua.unloadScript(scriptPath);
	}

	bool pressSettingsButton(std::string const &moduleKey, std::string const &settingId)
	{
		return m_lua.pressButton(moduleKey, settingId);
	}

	void updateMouse(int x, int y, bool pressed, bool released);
	bool wantsMouse() const;

	void tick(
		float deltaSeconds,
		int boundsWidth,
		int boundsHeight,
		std::vector<PDRect> const &monitors);
	void draw(PDSpriteRenderer &renderer) const;

private:
	static constexpr int MaxScriptErrors = 3;

	PDPonyPackData const *pack(std::string const &packPath);
	void advanceAnimations(float deltaSeconds);
	void writeSpriteFromAnimation(PDAnimation const &animation, PDSprite &sprite) const;

	bool playAnimation(entt::entity entity, std::string const &name, bool loop, bool restart);
	void setFacing(entt::entity entity, bool facingRight);
	entt::entity resolve(std::uint32_t entityId) const;

	PDDiagnostics *m_diagnostics = nullptr;
	PDTextureCache m_textures;
	PDTextCache m_labels;
	PDAnimationCache m_animations;
	std::unordered_map<std::string, PDPonyPackData> m_packs;
	PDLua m_lua;
	entt::registry m_registry;

	entt::entity m_hovered = entt::null;
	entt::entity m_dragged = entt::null;
	float m_dragOffsetX = 0.0f;
	float m_dragOffsetY = 0.0f;
};
