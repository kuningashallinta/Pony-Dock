#include <Engine/PDScene.h>

#include <Library/PDPonyPackLoader.h>

#include <cmath>
#include <filesystem>

PDScene::~PDScene()
{
	m_registry.clear();
}

void PDScene::initialize(
	ID3D11Device *device,
	PDDiagnostics &diagnostics,
	PDSettingsStore &settings,
	std::string const &scriptsRoot)
{
	m_diagnostics = &diagnostics;
	m_textures.initialize(device);
	m_labels.initialize(device);
	m_animations.initialize(m_textures, diagnostics);
	m_lua.initialize(scriptsRoot, diagnostics, settings);

	m_lua.setPlayHandler([this](std::uint32_t entityId, std::string const &name, bool loop, bool restart)
	{
		return playAnimation(resolve(entityId), name, loop, restart);
	});

	m_lua.setFacingHandler([this](std::uint32_t entityId, bool facingRight)
	{
		setFacing(resolve(entityId), facingRight);
	});
}

void PDScene::reloadPack(std::string const &packPath)
{
	std::string const key = std::filesystem::path(packPath).lexically_normal().string();
	auto const existing = m_packs.find(key);

	if (existing == m_packs.end())
	{
		return;
	}

	PDPonyPackData loaded;
	std::string error;

	if (not loadPonyPack(key, loaded, error))
	{
		m_diagnostics->write("Pack reload failed: " + error);

		return;
	}

	existing->second = std::move(loaded);
	m_lua.refreshPackTable(existing->second);

	auto view = m_registry.view<PDPack const, PDScript>();

	for (auto const entity : view)
	{
		if (view.get<PDPack const>(entity).data != &existing->second)
		{
			continue;
		}

		PDScript &script = view.get<PDScript>(entity);
		script.modules.clear();
	}

	m_diagnostics->write("Reloaded pack " + existing->second.id);
}

void PDScene::updateMouse(int x, int y, bool pressed, bool released)
{
	const float pointX = static_cast<float>(x);
	const float pointY = static_cast<float>(y);

	if (released)
	{
		m_dragged = entt::null;
	}

	m_hovered = entt::null;
	auto view = m_registry.view<PDPosition const, PDSprite const>();

	for (auto const entity : view)
	{
		PDPosition const &position = view.get<PDPosition const>(entity);
		PDSprite const &sprite = view.get<PDSprite const>(entity);

		const float left = position.x - sprite.offsetX;
		const float top = position.y - sprite.offsetY;

		if (pointX < left or pointX > left + sprite.width)
		{
			continue;
		}

		if (pointY < top or pointY > top + sprite.height)
		{
			continue;
		}

		m_hovered = entity;
	}

	if (pressed and m_registry.valid(m_hovered))
	{
		PDPosition const &position = m_registry.get<PDPosition>(m_hovered);
		m_dragged = m_hovered;
		m_dragOffsetX = pointX - position.x;
		m_dragOffsetY = pointY - position.y;
	}

	if (not m_registry.valid(m_dragged))
	{
		return;
	}

	PDPosition &position = m_registry.get<PDPosition>(m_dragged);
	position.x = pointX - m_dragOffsetX;
	position.y = pointY - m_dragOffsetY;
}

bool PDScene::wantsMouse() const
{
	return m_registry.valid(m_hovered) or m_registry.valid(m_dragged);
}

entt::entity PDScene::resolve(std::uint32_t entityId) const
{
	entt::entity const entity = static_cast<entt::entity>(entityId);

	return m_registry.valid(entity) ? entity : entt::null;
}

void PDScene::reloadScripts()
{
	m_lua.reload();

	auto view = m_registry.view<PDScript>();

	for (auto const entity : view)
	{
		PDScript &script = view.get<PDScript>(entity);
		script.modules.clear();
	}
}

PDPonyPackData const *PDScene::pack(std::string const &packPath)
{
	std::string const key = std::filesystem::path(packPath).lexically_normal().string();

	auto const existing = m_packs.find(key);

	if (existing != m_packs.end())
	{
		return existing->second.valid ? &existing->second : nullptr;
	}

	PDPonyPackData loaded;
	std::string error;

	if (not loadPonyPack(key, loaded, error))
	{
		m_diagnostics->writeOnce(key, "Pack load failed: " + error);
		m_packs.emplace(key, PDPonyPackData());

		return nullptr;
	}

	m_diagnostics->write("Loaded pack " + loaded.id + " (" + std::to_string(loaded.animations.size()) + " animations, " + std::to_string(loaded.behaviors.size()) + " behaviors)");

	return &m_packs.emplace(key, std::move(loaded)).first->second;
}

void PDScene::spawnEntity(std::string const &packPath, float x, float y)
{
	PDPonyPackData const *const packData = pack(packPath);

	if (packData == nullptr)
	{
		return;
	}

	PDTexture const *const preview = m_textures.texture(packData->previewPath);

	entt::entity const entity = m_registry.create();
	m_registry.emplace<PDPosition>(entity, x, y);
	m_registry.emplace<PDSprite>(entity, preview, 96.0f, 96.0f);
	m_registry.emplace<PDPack>(entity, packData);
	m_registry.emplace<PDAnimation>(entity);
	m_registry.emplace<PDLabel>(entity);

	PDScript &script = m_registry.emplace<PDScript>(entity);
	script.self = m_lua.createSelf(static_cast<std::uint32_t>(entity), packData->id);
	script.self["pack"] = m_lua.packTable(*packData);

	playAnimation(entity, packData->behaviors.front().animation, true, true);
}

bool PDScene::playAnimation(entt::entity entity, std::string const &name, bool loop, bool restart)
{
	if (not m_registry.valid(entity))
	{
		return false;
	}

	PDPack const *const packComponent = m_registry.try_get<PDPack>(entity);
	PDAnimation *const animation = m_registry.try_get<PDAnimation>(entity);

	if (packComponent == nullptr or packComponent->data == nullptr or animation == nullptr)
	{
		return false;
	}

	auto const found = packComponent->data->animations.find(name);

	if (found == packComponent->data->animations.end())
	{
		m_diagnostics->writeOnce(
			packComponent->data->id + "/" + name,
			"Unknown animation '" + name + "' in pack " + packComponent->data->id);

		return false;
	}

	std::string const &path = animation->facingRight ? found->second.right : found->second.left;
	PDAnimationClip const *const clip = m_animations.clip(path);

	if (clip == nullptr)
	{
		return false;
	}

	bool const unchanged = animation->clip == clip and animation->name == name and animation->loop == loop;

	if (unchanged and not restart)
	{
		return true;
	}

	animation->clip = clip;
	animation->name = name;
	animation->loop = loop;
	animation->finished = false;
	animation->frame = 0;
	animation->elapsedSeconds = 0.0f;

	PDSprite *const sprite = m_registry.try_get<PDSprite>(entity);

	if (sprite != nullptr)
	{
		writeSpriteFromAnimation(*animation, *sprite);
	}

	return true;
}

void PDScene::setFacing(entt::entity entity, bool facingRight)
{
	if (not m_registry.valid(entity))
	{
		return;
	}

	PDAnimation *const animation = m_registry.try_get<PDAnimation>(entity);

	if (animation == nullptr or animation->facingRight == facingRight)
	{
		return;
	}

	PDPack const *const packComponent = m_registry.try_get<PDPack>(entity);
	animation->facingRight = facingRight;

	if (packComponent == nullptr or packComponent->data == nullptr or animation->name.empty())
	{
		return;
	}

	auto const found = packComponent->data->animations.find(animation->name);

	if (found == packComponent->data->animations.end())
	{
		return;
	}

	PDAnimationClip const *const clip = m_animations.clip(facingRight ? found->second.right : found->second.left);

	if (clip == nullptr or clip == animation->clip)
	{
		return;
	}

	animation->clip = clip;
	animation->frame = animation->frame < static_cast<int>(clip->frames.size())
		? animation->frame
		: static_cast<int>(clip->frames.size()) - 1;

	float const frameDuration = clip->frames[animation->frame].durationSeconds;
	animation->elapsedSeconds = animation->elapsedSeconds < frameDuration ? animation->elapsedSeconds : frameDuration;

	PDSprite *const sprite = m_registry.try_get<PDSprite>(entity);

	if (sprite != nullptr)
	{
		writeSpriteFromAnimation(*animation, *sprite);
	}
}

void PDScene::writeSpriteFromAnimation(PDAnimation const &animation, PDSprite &sprite) const
{
	if (animation.clip == nullptr or animation.clip->frames.empty())
	{
		return;
	}

	PDAnimationFrame const &frame = animation.clip->frames[animation.frame];

	sprite.texture = animation.clip->atlas;
	sprite.width = frame.width;
	sprite.height = frame.height;
	sprite.u0 = frame.u0;
	sprite.v0 = frame.v0;
	sprite.u1 = frame.u1;
	sprite.v1 = frame.v1;
	sprite.offsetX = animation.clip->pivotX;
	sprite.offsetY = animation.clip->pivotY;
}

void PDScene::advanceAnimations(float deltaSeconds)
{
	auto view = m_registry.view<PDAnimation, PDSprite>();

	for (auto const entity : view)
	{
		PDAnimation &animation = view.get<PDAnimation>(entity);
		PDSprite &sprite = view.get<PDSprite>(entity);

		if (animation.clip == nullptr or animation.clip->frames.empty())
		{
			continue;
		}

		std::vector<PDAnimationFrame> const &frames = animation.clip->frames;

		if (not animation.finished)
		{
			animation.elapsedSeconds += deltaSeconds;

			int guard = static_cast<int>(frames.size()) + 1;

			while (guard > 0 and animation.elapsedSeconds >= frames[animation.frame].durationSeconds)
			{
				animation.elapsedSeconds -= frames[animation.frame].durationSeconds;
				guard -= 1;

				if (animation.frame + 1 < static_cast<int>(frames.size()))
				{
					animation.frame += 1;
				}
				else if (animation.loop)
				{
					animation.frame = 0;
				}
				else
				{
					animation.finished = true;
					animation.elapsedSeconds = 0.0f;

					break;
				}
			}
		}

		writeSpriteFromAnimation(animation, sprite);
	}
}

void PDScene::clear()
{
	m_hovered = entt::null;
	m_dragged = entt::null;
	m_registry.clear();
}

void PDScene::tick(
	float deltaSeconds,
	int boundsWidth,
	int boundsHeight,
	std::vector<PDRect> const &monitors)
{
	advanceAnimations(deltaSeconds);

	m_lua.beginFrame(static_cast<float>(boundsWidth), static_cast<float>(boundsHeight), monitors);

	std::vector<std::string> const scripts = m_lua.modules();
	auto view = m_registry.view<PDPosition, PDSprite const, PDAnimation const, PDScript, PDLabel>();

	for (auto const entity : view)
	{
		PDScript &script = view.get<PDScript>(entity);
		PDPosition &position = view.get<PDPosition>(entity);
		PDSprite const &sprite = view.get<PDSprite const>(entity);
		PDAnimation const &animation = view.get<PDAnimation const>(entity);

		script.self["x"] = position.x;
		script.self["y"] = position.y;
		script.self["width"] = sprite.width;
		script.self["height"] = sprite.height;
		script.self["offset_x"] = sprite.offsetX;
		script.self["offset_y"] = sprite.offsetY;
		script.self["facing"] = animation.facingRight ? "right" : "left";
		script.self["animation"] = animation.name;
		script.self["animation_finished"] = animation.finished;
		script.self["mouse_over"] = entity == m_hovered;
		script.self["dragged"] = entity == m_dragged;

		for (std::string const &path : scripts)
		{
			PDScript::Module &state = script.modules[path];

			if (state.errorCount >= MaxScriptErrors)
			{
				continue;
			}

			bool succeeded = true;

			if (not state.spawned)
			{
				succeeded = m_lua.callSpawn(path, script.self);
				state.spawned = true;
			}

			if (succeeded)
			{
				succeeded = m_lua.callTick(path, script.self, deltaSeconds);
			}

			if (not succeeded)
			{
				state.errorCount += 1;

				if (state.errorCount >= MaxScriptErrors)
				{
					m_diagnostics->write("Module disabled after " + std::to_string(MaxScriptErrors) + " errors: " + path);
				}

				continue;
			}

			state.errorCount = 0;
		}

		PDLabel &label = view.get<PDLabel>(entity);
		std::string const text = script.self["label"].get_or(std::string());

		if (text != label.text)
		{
			label.text = text;
			label.texture = text.empty() ? nullptr : m_labels.texture(text);
		}

		if (entity == m_dragged)
		{
			continue;
		}

		position.x = script.self["x"].get_or(position.x);
		position.y = script.self["y"].get_or(position.y);
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

		renderer.draw(
			sprite.texture->view(),
			std::floor(position.x - sprite.offsetX + 0.5f),
			std::floor(position.y - sprite.offsetY + 0.5f),
			sprite.width,
			sprite.height,
			sprite.u0,
			sprite.v0,
			sprite.u1,
			sprite.v1);
	}

	auto labels = m_registry.view<PDPosition const, PDSprite const, PDLabel const>();

	for (auto const entity : labels)
	{
		PDLabel const &label = labels.get<PDLabel const>(entity);

		if (label.texture == nullptr or not label.texture->valid())
		{
			continue;
		}

		PDPosition const &position = labels.get<PDPosition const>(entity);
		PDSprite const &sprite = labels.get<PDSprite const>(entity);
		float const width = static_cast<float>(label.texture->width());
		float const height = static_cast<float>(label.texture->height());

		renderer.draw(
			label.texture->view(),
			std::floor(position.x - sprite.offsetX + (sprite.width - width) * 0.5f + 0.5f),
			std::floor(position.y - sprite.offsetY - height - 4.0f + 0.5f),
			width,
			height,
			0.0f,
			0.0f,
			1.0f,
			1.0f);
	}
}
