#include "PipelineState.h"
#include "Engine.h"
#include <d3dcompiler.h>

#pragma comment(lib,"d3dcompiler.lib")

PipelineState::PipelineState()
{
	// �p�C�v���C���X�e�[�g�̐ݒ�
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // ���X�^���C�U�[�̓f�t�H���g
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // �J�����O�͂Ȃ�
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // �u�����h�X�e�[�g���f�t�H���g
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // �[�x�X�e���V���̓f�t�H���g���g��
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // �O�p�`�`��
	desc.NumRenderTargets = 1; // �`��Ώۂ�1
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1; // �T���v���[��1
	desc.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�
}

bool PipelineState::IsValid()
{
	return m_IsValid;
}

void PipelineState::SetInputLayout(D3D12_INPUT_LAYOUT_DESC layout)
{
	desc.InputLayout = layout;
}

void PipelineState::SetRootSignature(ID3D12RootSignature* rootSignature)
{
	desc.pRootSignature = rootSignature;
}

void PipelineState::SetVS(std::wstring filePath)
{
	// ���_�V�F�[�_�[�ǂݍ���
	auto hr = D3DReadFileToBlob(filePath.c_str(), m_pVsBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("���_�V�F�[�_�[�̓ǂݍ��݂Ɏ��s\n");
		return;
	}
	else
	{
		printf("���_�V�F�[�_�[�̓ǂݍ��݂ɐ���\n");
	}

	desc.VS = CD3DX12_SHADER_BYTECODE(m_pVsBlob.Get());
}

void PipelineState::SetPS(std::wstring filePath)
{
	// �s�N�Z���V�F�[�_�[�ǂݍ���
	auto hr = D3DReadFileToBlob(filePath.c_str(), m_pPsBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("�s�N�Z���V�F�[�_�[�̓ǂݍ��݂Ɏ��s\n");
		return;
	}
	else
	{
		printf("�s�N�Z���V�F�[�_�[�̓ǂݍ��݂ɐ���\n");
	}

	desc.PS = CD3DX12_SHADER_BYTECODE(m_pPsBlob.Get());
}

void PipelineState::Create()
{
	// �p�C�v���C���X�e�[�g�𐶐�
	auto hr = g_Engine->Device()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		printf("�p�C�v���C���X�e�[�g�̐����Ɏ��s\n");
		return;
	}
	else
	{
		printf("�p�C�v���C���X�e�[�g�̐����ɐ���\n");
	}

	m_IsValid = true;
}

ID3D12PipelineState* PipelineState::Get()
{
	return m_pPipelineState.Get();
}
