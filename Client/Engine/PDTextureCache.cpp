#include <Engine/PDTextureCache.h>

void PDTextureCache::initialize(ID3D11Device *device)
{
	m_device = device;
}

PDTexture const *PDTextureCache::texture(std::string const &path)
{
	auto const existing = m_textures.find(path);

	if (existing != m_textures.end())
	{
		return &existing->second;
	}

	return &m_textures.emplace(path, PDTexture(m_device, path)).first->second;
}
