#pragma once
#include <d3dx12.h>
#include "ComPtr.h"

class ConstantBuffer
{
public:
	ConstantBuffer(size_t size); // �R���X�g���N�^�Ńo�b�t�@�𐶐�
	bool IsValid(); // �o�b�t�@�����ɐ�����������Ԃ�
	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const; // �o�b�t�@��GPU��̃A�h���X��Ԃ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc(); // �萔�o�b�t�@�r���[��Ԃ�

	void* GetPtr() const; // �萔�o�b�t�@�Ƀ}�b�s���O���ꂽ�|�C���^��Ԃ�

	template<typename T>
	T* GetPtr()
	{
		return reinterpret_cast<T*>(GetPtr());
	}

private:
	bool m_IsValid = false; // �萔�o�b�t�@�����Ɏ��s������
	ComPtr<ID3D12Resource> m_pBuffer; // �萔�o�b�t�@
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_Desc; // �萔�o�b�t�@�r���[�̐ݒ�
	void* m_pMappedPtr = nullptr;

	ConstantBuffer(const ConstantBuffer&) = delete;
	void operator = (const ConstantBuffer&) = delete;
};