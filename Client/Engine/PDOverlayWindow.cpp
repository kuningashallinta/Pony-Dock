#include <Engine/PDOverlayWindow.h>

#include <dwmapi.h>

bool PDOverlayWindow::initialize(HINSTANCE instance)
{
	WNDCLASSEX windowClass = {sizeof(WNDCLASSEX), CS_CLASSDC, staticWndProc, 0, 0, instance, nullptr, nullptr,
		nullptr, nullptr, "PonyDockOverlayClassName", nullptr};
	RegisterClassEx(&windowClass);

	m_width = GetSystemMetrics(SM_CXSCREEN);
	m_height = GetSystemMetrics(SM_CYSCREEN);

	DWORD const exStyle = WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
	m_window = CreateWindowEx(exStyle, windowClass.lpszClassName, "", WS_POPUP, 0, 0, m_width, m_height, nullptr, nullptr, instance, nullptr);

	SetWindowLongPtr(m_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	MARGINS const margins = {-1, -1, -1, -1};
	DwmExtendFrameIntoClientArea(m_window, &margins);

	if (not createDevice())
	{
		destroyDevice();
		UnregisterClass(windowClass.lpszClassName, instance);

		return false;
	}

	ShowWindow(m_window, SW_SHOWNA);

	return true;
}

void PDOverlayWindow::shutdown()
{
	destroyDevice();

	HINSTANCE const instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_window, GWLP_HINSTANCE));
	DestroyWindow(m_window);
	UnregisterClass("PonyDockOverlayClassName", instance);

	m_window = nullptr;
}

void PDOverlayWindow::pumpMessages()
{
	MSG message;

	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

void PDOverlayWindow::beginFrame()
{
	constexpr float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, clearColor);

	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(m_width);
	viewport.Height = static_cast<float>(m_height);
	viewport.MaxDepth = 1.0f;
	m_deviceContext->RSSetViewports(1, &viewport);
}

void PDOverlayWindow::endFrame()
{
	m_swapChain->Present(1, 0);
}

bool PDOverlayWindow::createDevice()
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = m_window;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL featureLevel;
	constexpr D3D_FEATURE_LEVEL featureLevelArray[] = {D3D_FEATURE_LEVEL_11_0};

	HRESULT const result = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		featureLevelArray,
		1,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&m_swapChain,
		&m_device,
		&featureLevel,
		&m_deviceContext);

	if (result != S_OK)
	{
		return false;
	}

	createRenderTarget();

	return true;
}

void PDOverlayWindow::destroyDevice()
{
	destroyRenderTarget();

	if (m_swapChain != nullptr)
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}

	if (m_deviceContext != nullptr)
	{
		m_deviceContext->Release();
		m_deviceContext = nullptr;
	}

	if (m_device != nullptr)
	{
		m_device->Release();
		m_device = nullptr;
	}
}

void PDOverlayWindow::createRenderTarget()
{
	ID3D11Texture2D *backBuffer = nullptr;
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
	backBuffer->Release();
}

void PDOverlayWindow::destroyRenderTarget()
{
	if (m_renderTargetView != nullptr)
	{
		m_renderTargetView->Release();
		m_renderTargetView = nullptr;
	}
}

LRESULT CALLBACK PDOverlayWindow::staticWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	PDOverlayWindow *const instance = reinterpret_cast<PDOverlayWindow *>(GetWindowLongPtr(window, GWLP_USERDATA));

	if (instance != nullptr)
	{
		return instance->handleMessage(window, message, wParam, lParam);
	}

	return DefWindowProc(window, message, wParam, lParam);
}

LRESULT PDOverlayWindow::handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_NCHITTEST:
		{
			return HTTRANSPARENT;
		}

		case WM_DESTROY:
		{
			return 0;
		}
	}

	return DefWindowProc(window, message, wParam, lParam);
}
