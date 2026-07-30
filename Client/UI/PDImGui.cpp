#include <UI/PDImGui.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	PDImGui *s_instance = nullptr;
}

void PDImGui::setFrameCallback(std::function<void()> callback)
{
	m_frameCallback = std::move(callback);
}

bool PDImGui::initialize(HINSTANCE instance, const char *title, int width, int height)
{
	s_instance = this;

	WNDCLASSEX windowClass = {sizeof(WNDCLASSEX), CS_CLASSDC, staticWndProc, 0, 0, instance, nullptr, nullptr,
		nullptr, nullptr, "PonyDockClassName", nullptr};
	RegisterClassEx(&windowClass);

	m_window = CreateWindow(windowClass.lpszClassName, title, WS_OVERLAPPEDWINDOW, 100, 100, width, height, nullptr,
		nullptr, instance, nullptr);

	if (not createDevice())
	{
		destroyDevice();
		UnregisterClass(windowClass.lpszClassName, instance);

		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	char windowsDirectory[MAX_PATH];
	GetWindowsDirectoryA(windowsDirectory, MAX_PATH);
	std::string const fontPath = std::string(windowsDirectory) + "\\Fonts\\segoeui.ttf";

	if (ImFont *font = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f))
	{
		ImGui::GetIO().FontDefault = font;
	}

	ImGui_ImplWin32_Init(m_window);
	ImGui_ImplDX11_Init(m_device, m_deviceContext);

	ShowWindow(m_window, SW_SHOWDEFAULT);
	UpdateWindow(m_window);

	return true;
}

void PDImGui::shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	destroyDevice();

	HINSTANCE const instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_window, GWLP_HINSTANCE));
	DestroyWindow(m_window);
	UnregisterClass("PonyDockClassName", instance);

	m_window = nullptr;
	s_instance = nullptr;
}

bool PDImGui::pumpMessages()
{
	MSG message;

	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			return false;
		}
	}

	return true;
}

void PDImGui::close()
{
	DestroyWindow(m_window);
}

void PDImGui::beginFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void PDImGui::endFrame()
{
	ImGui::Render();

	constexpr float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, clearColor);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	m_swapChain->Present(1, 0);
}

bool PDImGui::createDevice()
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

	if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 1,
			D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, &featureLevel, &m_deviceContext) != S_OK)
	{
		return false;
	}

	createRenderTarget();

	return true;
}

void PDImGui::destroyDevice()
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

void PDImGui::createRenderTarget()
{
	ID3D11Texture2D *backBuffer = nullptr;
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
	backBuffer->Release();
}

void PDImGui::destroyRenderTarget()
{
	if (m_renderTargetView != nullptr)
	{
		m_renderTargetView->Release();
		m_renderTargetView = nullptr;
	}
}

void PDImGui::renderFrame()
{
	if (not m_frameCallback)
	{
		return;
	}

	beginFrame();
	m_frameCallback();
	endFrame();
}

LRESULT CALLBACK PDImGui::staticWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (s_instance != nullptr)
	{
		return s_instance->handleMessage(window, message, wParam, lParam);
	}

	return DefWindowProc(window, message, wParam, lParam);
}

LRESULT PDImGui::handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam) != 0)
	{
		return true;
	}

	switch (message)
	{
		case WM_SIZE:
			if (m_device != nullptr and wParam != SIZE_MINIMIZED)
			{
				destroyRenderTarget();
				m_swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				createRenderTarget();
				renderFrame();
			}

			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);

			return 0;
	}

	return DefWindowProc(window, message, wParam, lParam);
}
