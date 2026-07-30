#pragma once

#include <Engine/PDTexture.h>

#include <d3d11.h>

#include <string>
#include <unordered_map>

class PDTextureCache
{
public:
	void initialize(ID3D11Device *device);

	PDTexture const *texture(std::string const &path);

private:
	ID3D11Device *m_device = nullptr;
	std::unordered_map<std::string, PDTexture> m_textures;
};
