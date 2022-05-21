#include "Engine.h"
#include <d3d12.h>
#include <stdio.h>
#include <Windows.h>

Engine* g_Engine;

bool Engine::Init(HWND hwnd, UINT window_width, UINT window_height)
{
	m_FrameBufferWidth = window_width;
	m_FrameBufferHeight = window_height;
	m_hWnd = hwnd;

	printf("•`‰æƒGƒ“ƒWƒ“‚Ì‰Šú‰»‚É¬Œ÷\n");
	return true;
}

void Engine::BeginRender()
{
}

void Engine::EndRender()
{
}

ID3D12Device6* Engine::Device()
{
	return nullptr;
}

ID3D12GraphicsCommandList* Engine::CommandList()
{
	return nullptr;
}

UINT Engine::CurrentBackBufferIndex()
{
	return 0;
}

bool Engine::CreateDevice()
{
	return false;
}

bool Engine::CreateCommandQueue()
{
	return false;
}

bool Engine::CreateSwapChain()
{
	return false;
}

bool Engine::CreateCommandList()
{
	return false;
}

bool Engine::CreateFence()
{
	return false;
}

void Engine::CreateViewPort()
{
}

void Engine::CreateScissorRect()
{
}

bool Engine::CreateRenderTarget()
{
	return false;
}

bool Engine::CreateDepthStencil()
{
	return false;
}

void Engine::WaitRender()
{
}
