#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3d11.h>
#include <windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	ID3D11Device *g_Device = nullptr;
	ID3D11DeviceContext *g_DeviceContext = nullptr;
	IDXGISwapChain *g_SwapChain = nullptr;
	ID3D11RenderTargetView *g_RenderTargetView = nullptr;

	void createRenderTarget()
	{
		ID3D11Texture2D *backBuffer = nullptr;
		g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTargetView);
		backBuffer->Release();
	}

	void destroyRenderTarget()
	{
		if (g_RenderTargetView != nullptr)
		{
			g_RenderTargetView->Release();
			g_RenderTargetView = nullptr;
		}
	}

	bool createDevice(HWND window)
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = window;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		D3D_FEATURE_LEVEL featureLevel;
		constexpr D3D_FEATURE_LEVEL featureLevelArray[] = {D3D_FEATURE_LEVEL_11_0};

		if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 1,
				D3D11_SDK_VERSION, &swapChainDesc, &g_SwapChain, &g_Device, &featureLevel, &g_DeviceContext) != S_OK)
		{
			return false;
		}

		createRenderTarget();

		return true;
	}

	void destroyDevice()
	{
		destroyRenderTarget();

		if (g_SwapChain != nullptr)
		{
			g_SwapChain->Release();
			g_SwapChain = nullptr;
		}

		if (g_DeviceContext != nullptr)
		{
			g_DeviceContext->Release();
			g_DeviceContext = nullptr;
		}

		if (g_Device != nullptr)
		{
			g_Device->Release();
			g_Device = nullptr;
		}
	}

	LRESULT CALLBACK wndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam) != 0)
		{
			return true;
		}

		switch (message)
		{
		case WM_SIZE:
			if (g_Device != nullptr and wParam != SIZE_MINIMIZED)
			{
				destroyRenderTarget();
				g_SwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				createRenderTarget();
			}

			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);

			return 0;
		}

		return DefWindowProc(window, message, wParam, lParam);
	}

	void buildUI()
	{
		ImGuiViewport const *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		ImGui::Begin("Pony Dock", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::BeginChild("Sidebar", ImVec2(96, 0), ImGuiChildFlags_Borders);
		ImGui::Button("BROWSER", ImVec2(64, 64));
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("Content", ImVec2(0, 0));
		ImGui::BeginChild("Toolbar", ImVec2(0, 64), ImGuiChildFlags_Borders);
		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 128 - 4, (64 - 32) / 2));
		ImGui::Button("START", ImVec2(128, 32));
		ImGui::EndChild();
		ImGui::EndChild();

		ImGui::End();
	}
} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	WNDCLASSEX windowClass = {sizeof(WNDCLASSEX), CS_CLASSDC, wndProc, 0, 0, instance, nullptr, nullptr, nullptr,
		nullptr, "PonyDockClassName", nullptr};
	RegisterClassEx(&windowClass);

	HWND window = CreateWindow(windowClass.lpszClassName, "Pony Dock", WS_OVERLAPPEDWINDOW, 100, 100, 720, 480,
		nullptr, nullptr, windowClass.hInstance, nullptr);

	if (not createDevice(window))
	{
		destroyDevice();
		UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

		return 1;
	}

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(g_Device, g_DeviceContext);

	bool running = true;

	while (running)
	{
		MSG message;

		while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);

			if (message.message == WM_QUIT)
			{
				running = false;
			}
		}

		if (not running)
		{
			break;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		buildUI();

		ImGui::Render();

		constexpr float clearColor[4] = {0.122f, 0.129f, 0.145f, 1.0f};
		g_DeviceContext->OMSetRenderTargets(1, &g_RenderTargetView, nullptr);
		g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, clearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_SwapChain->Present(1, 0);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	destroyDevice();
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

	return 0;
}
