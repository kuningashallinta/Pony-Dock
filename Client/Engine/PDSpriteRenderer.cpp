#include <Engine/PDSpriteRenderer.h>

#include <Math/PDVertex2D.h>

#include <d3dcompiler.h>

#include <cstring>
#include <string_view>

static std::string_view const VertexShaderSource = R"(
		struct VSInput { float2 pos : POSITION; float2 uv : TEXCOORD0; };
		struct PSInput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

		PSInput main(VSInput input)
		{
			PSInput output;
			output.pos = float4(input.pos, 0.0f, 1.0f);
			output.uv = input.uv;
			return output;
		}
	)";

static std::string_view const PixelShaderSource = R"(
		Texture2D spriteTexture : register(t0);
		SamplerState spriteSampler : register(s0);

		float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
		{
			return spriteTexture.Sample(spriteSampler, uv);
		}
	)";

bool PDSpriteRenderer::initialize(ID3D11Device *device)
{
	ID3DBlob *vertexBlob = nullptr;
	ID3DBlob *pixelBlob = nullptr;
	ID3DBlob *errorBlob = nullptr;

	if (D3DCompile(VertexShaderSource.data(), VertexShaderSource.size(), nullptr, nullptr, nullptr, "main", "vs_4_0",
			0, 0, &vertexBlob, &errorBlob) != S_OK)
	{
		if (errorBlob != nullptr)
		{
			errorBlob->Release();
		}

		return false;
	}

	if (D3DCompile(PixelShaderSource.data(), PixelShaderSource.size(), nullptr, nullptr, nullptr, "main", "ps_4_0", 0,
			0, &pixelBlob, &errorBlob) != S_OK)
	{
		vertexBlob->Release();

		if (errorBlob != nullptr)
		{
			errorBlob->Release();
		}

		return false;
	}

	device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &m_vertexShader);
	device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &m_pixelShader);

	D3D11_INPUT_ELEMENT_DESC const layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	device->CreateInputLayout(layout, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &m_inputLayout);

	vertexBlob->Release();
	pixelBlob->Release();

	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(PDVertex2D) * 4;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	device->CreateBuffer(&vertexBufferDesc, nullptr, &m_vertexBuffer);

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	device->CreateSamplerState(&samplerDesc, &m_sampler);

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&blendDesc, &m_blendState);

	return m_vertexShader != nullptr and m_pixelShader != nullptr and m_inputLayout != nullptr and m_vertexBuffer != nullptr and m_sampler != nullptr and m_blendState != nullptr;
}

void PDSpriteRenderer::shutdown()
{
	if (m_blendState != nullptr)
	{
		m_blendState->Release();
		m_blendState = nullptr;
	}

	if (m_sampler != nullptr)
	{
		m_sampler->Release();
		m_sampler = nullptr;
	}

	if (m_vertexBuffer != nullptr)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}

	if (m_inputLayout != nullptr)
	{
		m_inputLayout->Release();
		m_inputLayout = nullptr;
	}

	if (m_pixelShader != nullptr)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}

	if (m_vertexShader != nullptr)
	{
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}
}

void PDSpriteRenderer::begin(ID3D11DeviceContext *context, int screenWidth, int screenHeight)
{
	m_context = context;
	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;

	constexpr float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	context->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->IASetInputLayout(m_inputLayout);
	context->VSSetShader(m_vertexShader, nullptr, 0);
	context->PSSetShader(m_pixelShader, nullptr, 0);
	context->PSSetSamplers(0, 1, &m_sampler);

	UINT const stride = sizeof(PDVertex2D);
	UINT const offset = 0;
	context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
}

void PDSpriteRenderer::draw(ID3D11ShaderResourceView *texture, float x, float y, float width, float height)
{
	if (m_context == nullptr or texture == nullptr or m_screenWidth <= 0 or m_screenHeight <= 0)
	{
		return;
	}

	auto const toNdcX = [this](float pixelX)
	{
		return pixelX / static_cast<float>(m_screenWidth) * 2.0f - 1.0f;
	};
	auto const toNdcY = [this](float pixelY)
	{
		return 1.0f - pixelY / static_cast<float>(m_screenHeight) * 2.0f;
	};

	PDVertex2D const vertices[4] = {
		{toNdcX(x), toNdcY(y), 0.0f, 0.0f},
		{toNdcX(x + width), toNdcY(y), 1.0f, 0.0f},
		{toNdcX(x), toNdcY(y + height), 0.0f, 1.0f},
		{toNdcX(x + width), toNdcY(y + height), 1.0f, 1.0f},
	};

	D3D11_MAPPED_SUBRESOURCE mapped;

	if (m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) != S_OK)
	{
		return;
	}

	memcpy(mapped.pData, vertices, sizeof(vertices));
	m_context->Unmap(m_vertexBuffer, 0);

	m_context->PSSetShaderResources(0, 1, &texture);
	m_context->Draw(4, 0);
}
