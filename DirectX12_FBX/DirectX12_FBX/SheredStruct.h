#pragma once
#include <d3dx12.h>
#include <DirectXMath.h>
#include "ComPtr.h"

struct Vertex
{
	DirectX::XMFLOAT3 Position;	// �ʒu���W
	DirectX::XMFLOAT3 Normal;	// �@��
	DirectX::XMFLOAT2 uv;		// uv���W
	DirectX::XMFLOAT3 Tangent;	// �ڋ��
	DirectX::XMFLOAT4 Color;	// ���_�F

	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
	static const int InputElementCount = 5;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

struct alignas(256) Transform
{
	DirectX::XMMATRIX World; // ���[���h�s��
	DirectX::XMMATRIX View;  // �r���[�s��
	DirectX::XMMATRIX Proj;  // ���e�s��
};