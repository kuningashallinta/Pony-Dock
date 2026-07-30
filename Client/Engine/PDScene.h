#pragma once

#include <Engine/Components/PDPosition.h>
#include <Engine/Components/PDSprite.h>
#include <Engine/Components/PDVelocity.h>
#include <Engine/PDSpriteRenderer.h>
#include <Engine/PDTexture.h>

#include <d3d11.h>

#include <entt/entt.hpp>

#include <string>
#include <unordered_map>

class PDScene
{
public:
	void initialize(ID3D11Device *device);

	void spawnEntity(std::string const &previewPath, float x, float y);

	void tick(float deltaSeconds, int boundsWidth, int boundsHeight);
	void draw(PDSpriteRenderer &renderer) const;

private:
	PDTexture *texture(std::string const &path);

	ID3D11Device *m_device = nullptr;
	entt::registry m_registry;
	std::unordered_map<std::string, PDTexture> m_textures;
};
