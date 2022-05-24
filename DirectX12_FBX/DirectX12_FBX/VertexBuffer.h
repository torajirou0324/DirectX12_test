#pragma once
#include <d3dx12.h>
#include "ComPtr.h"

class VertexBuffer
{
public:
	VertexBuffer(size_t size, size_t stride, const void* pInitData); // コンストラクタでバッファを生成
	D3D12_VERTEX_BUFFER_VIEW View() const; // 頂点バッファーを取得
	bool IsValid(); // バッファの生成に成功したか取得

private:
	bool m_IsValid = false; // バッファの生成に成功したか取得
	ComPtr<ID3D12Resource> m_pBuffer = nullptr; // バッファ
	D3D12_VERTEX_BUFFER_VIEW m_View = {}; // 頂点バッファービュー

	VertexBuffer(const VertexBuffer&) = delete;
	void operator = (const VertexBuffer&) = delete;
};