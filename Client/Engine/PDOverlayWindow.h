#pragma once

#include <d3d11.h>
#include <windows.h>

class PDOverlayWindow
{
public:
	bool initialize(HINSTANCE instance);
	void shutdown();

	void pumpMessages();

	void beginFrame();
	void endFrame();

	ID3D11Device *device() const
	{
		return m_device;
	}

	ID3D11DeviceContext *context() const
	{
		return m_deviceContext;
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
	static LRESULT CALLBACK staticWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

	bool createDevice();
	void destroyDevice();
	void createRenderTarget();
	void destroyRenderTarget();

	HWND m_window = nullptr;
	int m_width = 0;
	int m_height = 0;

	ID3D11Device *m_device = nullptr;
	ID3D11DeviceContext *m_deviceContext = nullptr;
	IDXGISwapChain *m_swapChain = nullptr;
	ID3D11RenderTargetView *m_renderTargetView = nullptr;
};
