#pragma once
#include "ComPtr.h"

// ���[�g�V�O�l�`��
// �V�F�[�_�[���Ŏg�p����萔�o�b�t�@��e�N�X�`���A�T���v���[�Ȃǂ̃��\�[�X�̃��C�A�E�g�i��������ɂǂ�����ł��邩�j���`����I�u�W�F�N�g
// ���[�g�V�O�l�`�����g���āA�V�F�[�_�[���̂ǂ̃��W�X�^�[��GPU�̂ǂ̃������̓��e��R�Â��邩�����肷��
// �V�F�[�_�[�ɂ��낢��ȃf�[�^��n�������ꍇ�A���[�g�V�O�l�`���������Ǝg�����Ȃ��K�v������

struct ID3D12RootSignature;

class RootSignature
{
public:
	RootSignature(); // �R���X�g���N�^�Ń��[�g�V�O�l�`���𐶐�
	bool IsValid();  // ���[�g�V�O�l�`���̐����Ɏ��s�������ǂ�����Ԃ�
	ID3D12RootSignature* Get(); // ���[�g�V�O�l�`����Ԃ�

private:
	bool m_IsValid = false; // ���[�g�V�O�l�`���̐����ɐ����������ǂ���
	ComPtr<ID3D12RootSignature> m_pRootSignature = nullptr; // ���[�g�V�O�l�`��
};