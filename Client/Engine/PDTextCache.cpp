#include <Engine/PDTextCache.h>

#include <Core/PDString.h>

#include <algorithm>
#include <cmath>
#include <vector>

static std::vector<unsigned char> rasterize(
	ID3D11Device *device,
	ID2D1Factory *factory,
	IDWriteTextLayout *layout,
	int width,
	int height)
{
	D3D11_TEXTURE2D_DESC targetDesc = {};
	targetDesc.Width = static_cast<UINT>(width);
	targetDesc.Height = static_cast<UINT>(height);
	targetDesc.MipLevels = 1;
	targetDesc.ArraySize = 1;
	targetDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	targetDesc.SampleDesc.Count = 1;
	targetDesc.Usage = D3D11_USAGE_DEFAULT;
	targetDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

	ID3D11Texture2D *target = nullptr;
	device->CreateTexture2D(&targetDesc, nullptr, &target);

	if (target == nullptr)
	{
		return std::vector<unsigned char>();
	}

	IDXGISurface *surface = nullptr;
	ID2D1RenderTarget *renderTarget = nullptr;

	if (SUCCEEDED(target->QueryInterface(IID_PPV_ARGS(&surface))))
	{
		D2D1_RENDER_TARGET_PROPERTIES const properties = D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

		factory->CreateDxgiSurfaceRenderTarget(surface, properties, &renderTarget);
		surface->Release();
	}

	if (renderTarget == nullptr)
	{
		target->Release();

		return std::vector<unsigned char>();
	}

	ID2D1SolidColorBrush *pill = nullptr;
	ID2D1SolidColorBrush *ink = nullptr;
	renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.09f, 0.09f, 0.11f, 0.72f), &pill);
	renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &ink);

	renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	renderTarget->BeginDraw();
	renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

	if (pill != nullptr and ink != nullptr)
	{
		D2D1_RECT_F const bounds = D2D1::RectF(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

		renderTarget->FillRoundedRectangle(D2D1::RoundedRect(bounds, 7.0f, 7.0f), pill);
		renderTarget->DrawTextLayout(D2D1::Point2F(7.0f, 3.0f), layout, ink);
	}

	HRESULT const drawn = renderTarget->EndDraw();

	if (ink != nullptr)
	{
		ink->Release();
	}

	if (pill != nullptr)
	{
		pill->Release();
	}

	renderTarget->Release();

	targetDesc.Usage = D3D11_USAGE_STAGING;
	targetDesc.BindFlags = 0;
	targetDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	ID3D11Texture2D *staging = nullptr;
	device->CreateTexture2D(&targetDesc, nullptr, &staging);

	ID3D11DeviceContext *context = nullptr;
	device->GetImmediateContext(&context);

	std::vector<unsigned char> pixels;
	D3D11_MAPPED_SUBRESOURCE mapped = {};

	if (SUCCEEDED(drawn) and staging != nullptr and context != nullptr)
	{
		context->CopyResource(staging, target);

		if (SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)))
		{
			pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);

			for (int i = 0; i < height; i += 1)
			{
				unsigned char const *row = static_cast<unsigned char const *>(mapped.pData) + static_cast<std::size_t>(i) * mapped.RowPitch;

				for (int j = 0; j < width; j += 1)
				{
					unsigned char const *source = row + static_cast<std::size_t>(j) * 4;
					unsigned char *destination = pixels.data() + (static_cast<std::size_t>(i) * static_cast<std::size_t>(width) + static_cast<std::size_t>(j)) * 4;
					int const alpha = source[3];

					for (int k = 0; k < 3; k += 1)
					{
						destination[k] = alpha == 0 ? 0 : static_cast<unsigned char>(std::min(255, source[2 - k] * 255 / alpha));
					}

					destination[3] = static_cast<unsigned char>(alpha);
				}
			}

			context->Unmap(staging, 0);
		}
	}

	if (context != nullptr)
	{
		context->Release();
	}

	if (staging != nullptr)
	{
		staging->Release();
	}

	target->Release();

	return pixels;
}

PDTextCache::~PDTextCache()
{
	if (m_textFormat != nullptr)
	{
		m_textFormat->Release();
	}

	if (m_writeFactory != nullptr)
	{
		m_writeFactory->Release();
	}

	if (m_d2dFactory != nullptr)
	{
		m_d2dFactory->Release();
	}
}

void PDTextCache::initialize(ID3D11Device *device)
{
	m_device = device;

	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2dFactory)))
	{
		return;
	}

	HRESULT const created = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown **>(&m_writeFactory));

	if (FAILED(created))
	{
		return;
	}

	m_writeFactory->CreateTextFormat(
		L"Segoe UI",
		nullptr,
		DWRITE_FONT_WEIGHT_SEMI_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		14.0f,
		L"",
		&m_textFormat);
}

PDTexture const *PDTextCache::texture(std::string const &text)
{
	auto const existing = m_textures.find(text);

	if (existing != m_textures.end())
	{
		return &existing->second;
	}

	if (m_device == nullptr or m_textFormat == nullptr or text.empty())
	{
		return nullptr;
	}

	std::wstring const wide = toWide(text);
	IDWriteTextLayout *layout = nullptr;

	HRESULT const created = m_writeFactory->CreateTextLayout(
		wide.c_str(),
		static_cast<UINT32>(wide.size()),
		m_textFormat,
		480.0f,
		60.0f,
		&layout);

	if (FAILED(created))
	{
		return nullptr;
	}

	DWRITE_TEXT_METRICS metrics = {};
	layout->GetMetrics(&metrics);

	int const width = static_cast<int>(std::ceil(metrics.width + 7.0f * 2.0f));
	int const height = static_cast<int>(std::ceil(metrics.height + 3.0f * 2.0f));
	std::vector<unsigned char> const pixels = rasterize(m_device, m_d2dFactory, layout, width, height);
	layout->Release();

	if (pixels.empty())
	{
		return nullptr;
	}

	auto const inserted = m_textures.emplace(text, PDTexture(m_device, pixels.data(), width, height));

	return &inserted.first->second;
}
