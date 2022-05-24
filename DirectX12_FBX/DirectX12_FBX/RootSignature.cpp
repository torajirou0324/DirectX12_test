#include "RootSignature.h"
#include "Engine.h"
#include <d3dx12.h>

RootSignature::RootSignature()
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	// �A�v���P�[�V�����̓��̓A�Z���u�����g�p����
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;			// �h���C���V�F�[�_�[�̃��[�g�V�O�l�`���ւ�A�N�Z�X�����ۂ���
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;				// �n���V�F�[�_�[�̃��[�g�V�O�l�`���ւ�A�N�Z�X�����ۂ���
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;			// �W�I���g���V�F�[�_�[�̃��[�g�V�O�l�`���ւ�A�N�Z�X�����ۂ���

	CD3DX12_ROOT_PARAMETER rootParam[1] = {};
	rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // b0�̒萔�o�b�t�@��ݒ�A�S�ẴV�F�[�_�[���猩����悤�ɂ���

	// �X�^�e�B�b�N�T���v���[�̐ݒ�
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	// ���[�g�V�O�l�`���̐ݒ�(�ݒ肵�������[�g�p�����[�^�[�ƃX�^�e�B�b�N�T���v���[������)
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = std::size(rootParam); // ���[�g�p�����[�^�̌�������
	desc.NumStaticSamplers = 1; // �T���v���[�̌�������
	desc.pParameters = rootParam; // ���[�g�p�����[�^�[�̃|�C���^������
	desc.pStaticSamplers = &sampler; // �T���v���[�̃|�C���^������
	desc.Flags = flag; // �t���O��ݒ�

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	// �V���A���C�Y
	auto hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("���[�g�V�O�l�`���V���A���C�Y�Ɏ��s\n");
		return;
	}
	else
	{
		printf("���[�g�V�O�l�`���̃V���A���C�Y�ɐ���\n");
	}

	// ���[�g�V�O�l�`������
	hr = g_Engine->Device()->CreateRootSignature(
		0, // GPU����������ꍇ�̃m�[�h�}�X�N�i����͂P�����Ȃ��z��Ȃ̂�0�j
		pBlob->GetBufferPointer(), // �V���A���C�Y�����f�[�^�̃|�C���^
		pBlob->GetBufferSize(),    // �V���A���C�Y�����f�[�^�̃T�C�Y
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		printf("���[�g�V�O�l�`���̐����Ɏ��s\n");
		return;
	}
	else
	{
		printf("���[�g�V�O�l�`���̐����ɐ���\n");
	}

	m_IsValid = true;
}

bool RootSignature::IsValid()
{
	return m_IsValid;
}

ID3D12RootSignature* RootSignature::Get()
{
	return m_pRootSignature.Get();
}
