#include <Engine/PDScene.h>

void PDScene::initialize(ID3D11Device *device)
{
	m_device = device;
	m_lua.initialize();
}

PDTexture *PDScene::texture(std::string const &path)
{
	auto const existing = m_textures.find(path);

	if (existing != m_textures.end())
	{
		return &existing->second;
	}

	return &m_textures.emplace(path, PDTexture(m_device, path)).first->second;
}

void PDScene::spawnEntity(std::string const &previewPath, std::string const &scriptPath, float x, float y)
{
	entt::entity const entity = m_registry.create();
	m_registry.emplace<PDPosition>(entity, x, y);
	m_registry.emplace<PDVelocity>(entity, 60.0f, 0.0f);
	m_registry.emplace<PDSprite>(entity, texture(previewPath), 96.0f, 96.0f);
	m_registry.emplace<PDBehavior>(entity, scriptPath);
}

void PDScene::clear()
{
	m_registry.clear();
}

void PDScene::tick(float deltaSeconds, int boundsWidth, int boundsHeight)
{
	auto view = m_registry.view<PDPosition, PDVelocity, PDSprite const, PDBehavior const>();

	for (auto const entity : view)
	{
		PDPosition &position = view.get<PDPosition>(entity);
		PDVelocity &velocity = view.get<PDVelocity>(entity);
		PDSprite const &sprite = view.get<PDSprite const>(entity);
		PDBehavior const &behavior = view.get<PDBehavior const>(entity);

		m_lua.tick(
			behavior.scriptPath,
			position,
			velocity,
			deltaSeconds,
			static_cast<float>(boundsWidth),
			static_cast<float>(boundsHeight),
			sprite.width,
			sprite.height);
	}
}

void PDScene::draw(PDSpriteRenderer &renderer) const
{
	auto view = m_registry.view<PDPosition const, PDSprite const>();

	for (auto const entity : view)
	{
		PDPosition const &position = view.get<PDPosition const>(entity);
		PDSprite const &sprite = view.get<PDSprite const>(entity);

		if (sprite.texture == nullptr or not sprite.texture->valid())
		{
			continue;
		}

		renderer.draw(sprite.texture->view(), position.x, position.y, sprite.width, sprite.height);
	}
}
