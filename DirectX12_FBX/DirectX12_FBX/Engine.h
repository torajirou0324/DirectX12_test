#pragma once
#include <d3d12.h>
#include<dxgi.h>
#include<dxgi1_4.h>
#include "ComPtr.h"

#pragma comment(lib,"d3d12.lib")	// d3d12ライブラリをリンクする
#pragma comment(lib,"dxgi.lib")		// dxgiライブラリをリンクする

class Engine
{
public:
	enum { FRAME_BUFFER_COUNT = 2 }; // ダブルバッファリングをするので2

public:
	bool Init(HWND hwnd, UINT window_width, UINT window_height); // エンジン初期化

	void BeginRender();	// 描画の開始処理
	void EndRender();	// 描画の終了処理

public: // 外からアクセスしたいためGetterとして公開するもの
	ID3D12Device6* Device();
	ID3D12GraphicsCommandList* CommandList();
	UINT CurrentBackBufferIndex();

private: // DirectX12初期化に使う関数

	bool CreateDevice();		// デバイスを生成
	bool CreateCommandQueue();	// コマンドキューを生成
	bool CreateSwapChain();		// スワップチェインを生成
	bool CreateCommandList();	// コマンドリストとコマンドアローケーターを生成
	bool CreateFence();			// フェンスを生成
	void CreateViewPort();		// ビューポートを生成
	void CreateScissorRect();	// シザー短形を生成

private: // 描画に使用するオブジェクトたち

	HWND m_hWnd;
	UINT m_FrameBufferWidth = 0;
	UINT m_FrameBufferHeight = 0;
	UINT m_CurrentBackBufferIndex = 0;

	ComPtr<ID3D12Device> m_pDevice = nullptr;										// デバイス
	ComPtr<ID3D12CommandQueue> m_pQueue = nullptr;									// コマンドキュー
	ComPtr<IDXGISwapChain3> m_pSwapChain = nullptr;									// スワップチェイン
	ComPtr<ID3D12CommandAllocator> m_pAllowcator[FRAME_BUFFER_COUNT] = { nullptr };	// コマンドアローケーター
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList = nullptr;						// コマンドリスト
	HANDLE m_fenceEvent = nullptr;													// フェンスで使うイベント
	ComPtr<ID3D12Fence> m_pFence = nullptr;											// フェンス
	UINT64 m_fenceValue[FRAME_BUFFER_COUNT];										// フェンスの値（ダブルバッファリング用に2個）
	D3D12_VIEWPORT m_Viewport;														// ビューポート
	D3D12_RECT m_Scissor;															// シザー短形

private: // 描画に使用するオブジェクトと生成関数たち

	bool CreateRenderTarget(); // レンダーターゲットを生成
	bool CreateDepthStencil(); // 深度ステンシルバッファを生成

	UINT m_RtvDescriptorSize = 0;												// レンダーターゲットビューのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap = nullptr;							// レンダーターゲットのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pRenderTargets[FRAME_BUFFER_COUNT] = { nullptr };	// レンダーターゲット（ダブルバッファリングするので2個）

	UINT m_DsvDescriptorSize = 0;							// 深度ステンシルのディスクリプターサイズ
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr;		// 深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr;	// 深度ステンシルバッファ（こっちは1つ）

private: // 描画ループで使用するもの

	ID3D12Resource* m_currentRenderTarget = nullptr; // 現在のフレームのレンダーターゲットを一時的に保存しておく関数
	void WaitRender(); // 描画完了を待つs処理
};

extern Engine* g_Engine; // どこからでも参照したいためグローバルにしておく