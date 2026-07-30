#pragma once

#include <Engine/Components/PDAnimation.h>
#include <Engine/Components/PDBehavior.h>
#include <Engine/Components/PDPack.h>
#include <Engine/Components/PDPosition.h>
#include <Engine/Components/PDSprite.h>
#include <Engine/Components/PDVelocity.h>
#include <Engine/PDAnimationCache.h>
#include <Engine/PDDiagnostics.h>
#include <Engine/PDLua.h>
#include <Engine/PDSpriteRenderer.h>
#include <Engine/PDTextureCache.h>
#include <Library/PDPonyPackData.h>

#include <d3d11.h>

#include <entt/entt.hpp>

#include <string>
#include <unordered_map>

class PDScene
{
public:
	void initialize(ID3D11Device *device, PDDiagnostics &diagnostics);

	void spawnEntity(std::string const &packPath, std::string const &scriptPath, float x, float y);
	void clear();

	void tick(float deltaSeconds, int boundsWidth, int boundsHeight);
	void draw(PDSpriteRenderer &renderer) const;

	bool playAnimation(entt::entity entity, std::string const &name, bool loop, bool restart);
	void setFacing(entt::entity entity, bool facingRight);

private:
	PDPonyPackData const *pack(std::string const &packPath);
	void advanceAnimations(float deltaSeconds);
	void writeSpriteFromAnimation(PDAnimation const &animation, PDSprite &sprite) const;

	PDDiagnostics *m_diagnostics = nullptr;
	PDTextureCache m_textures;
	PDAnimationCache m_animations;
	std::unordered_map<std::string, PDPonyPackData> m_packs;
	PDLua m_lua;
	entt::registry m_registry;
};
