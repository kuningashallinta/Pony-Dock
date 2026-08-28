#pragma once

#include <Engine/PDTexture.h>

#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>

#include <string>
#include <unordered_map>

class PDTextCache
{
public:
	PDTextCache() = default;
	~PDTextCache();

	PDTextCache(PDTextCache const &) = delete;
	PDTextCache &operator=(PDTextCache const &) = delete;

	void initialize(ID3D11Device *device);

	PDTexture const *texture(std::string const &text);

private:
	ID3D11Device *m_device = nullptr;
	ID2D1Factory *m_d2dFactory = nullptr;
	IDWriteFactory *m_writeFactory = nullptr;
	IDWriteTextFormat *m_textFormat = nullptr;
	std::unordered_map<std::string, PDTexture> m_textures;
};
