#pragma once

#include <d3d11.h>

#include <string>

class PDTexture
{
public:
	PDTexture() = default;
	PDTexture(ID3D11Device *device, std::string const &path);
	~PDTexture();

	PDTexture(PDTexture const &) = delete;
	PDTexture &operator=(PDTexture const &) = delete;

	PDTexture(PDTexture &&other) noexcept;
	PDTexture &operator=(PDTexture &&other) noexcept;

	bool valid() const
	{
		return m_view != nullptr;
	}

	ID3D11ShaderResourceView *view() const
	{
		return m_view;
	}

	int width() const
	{
		return m_width;
	}

	int height() const
	{
		return m_height;
	}

private:
	void release();

	ID3D11ShaderResourceView *m_view = nullptr;
	int m_width = 0;
	int m_height = 0;
};
