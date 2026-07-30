#include <Engine/PDTexture.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <utility>

PDTexture::PDTexture(ID3D11Device *device, std::string const &path)
{
	int channels = 0;
	unsigned char *pixels = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);

	if (pixels == nullptr)
	{
		return;
	}

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = static_cast<UINT>(m_width);
	textureDesc.Height = static_cast<UINT>(m_height);
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = pixels;
	data.SysMemPitch = static_cast<UINT>(m_width) * 4;

	ID3D11Texture2D *texture = nullptr;
	device->CreateTexture2D(&textureDesc, &data, &texture);

	stbi_image_free(pixels);

	if (texture == nullptr)
	{
		m_width = 0;
		m_height = 0;

		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = textureDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(texture, &viewDesc, &m_view);
	texture->Release();

	if (m_view == nullptr)
	{
		m_width = 0;
		m_height = 0;
	}
}

PDTexture::~PDTexture()
{
	release();
}

PDTexture::PDTexture(PDTexture &&other) noexcept
	: m_view(std::exchange(other.m_view, nullptr)),
	  m_width(std::exchange(other.m_width, 0)),
	  m_height(std::exchange(other.m_height, 0))
{
}

PDTexture &PDTexture::operator=(PDTexture &&other) noexcept
{
	if (this != &other)
	{
		release();

		m_view = std::exchange(other.m_view, nullptr);
		m_width = std::exchange(other.m_width, 0);
		m_height = std::exchange(other.m_height, 0);
	}

	return *this;
}

void PDTexture::release()
{
	if (m_view != nullptr)
	{
		m_view->Release();
		m_view = nullptr;
	}
}
