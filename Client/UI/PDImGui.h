#pragma once

#include <d3d11.h>
#include <windows.h>

#include <functional>

class PDImGui
{
public:
	void setFrameCallback(std::function<void()> callback);

	bool initialize(HINSTANCE instance, const char *title, int width, int height);
	void shutdown();

	bool pumpMessages();
	void close();

	void renderFrame();

	HWND handle() const
	{
		return m_window;
	}

	ID3D11Device *device() const
	{
		return m_device;
	}

private:
	static LRESULT CALLBACK staticWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

	bool createDevice();
	void destroyDevice();
	void createRenderTarget();
	void destroyRenderTarget();
	void beginFrame();
	void endFrame();

	HWND m_window = nullptr;

	ID3D11Device *m_device = nullptr;
	ID3D11DeviceContext *m_deviceContext = nullptr;
	IDXGISwapChain *m_swapChain = nullptr;
	ID3D11RenderTargetView *m_renderTargetView = nullptr;

	std::function<void()> m_frameCallback;
};
