#pragma once

#include <d3d11.h>

class PDSpriteRenderer
{
public:
	bool initialize(ID3D11Device *device);
	void shutdown();

	void begin(ID3D11DeviceContext *context, int screenWidth, int screenHeight);

	void draw(
		ID3D11ShaderResourceView *texture,
		float x,
		float y,
		float width,
		float height,
		float u0,
		float v0,
		float u1,
		float v1);

private:
	ID3D11VertexShader *m_vertexShader = nullptr;
	ID3D11PixelShader *m_pixelShader = nullptr;
	ID3D11InputLayout *m_inputLayout = nullptr;
	ID3D11Buffer *m_vertexBuffer = nullptr;
	ID3D11SamplerState *m_sampler = nullptr;
	ID3D11BlendState *m_blendState = nullptr;

	ID3D11DeviceContext *m_context = nullptr;
	int m_screenWidth = 0;
	int m_screenHeight = 0;
};
