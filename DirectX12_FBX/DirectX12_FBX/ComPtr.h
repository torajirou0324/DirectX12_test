#pragma once
#include <wrl/client.h>

template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

// DirectX12�֘A�̃I�u�W�F�N�g�͏I�����ɉ��������������ƍs����������ComPtr���g�p����
// ��������Release�֐����Ă΂Ȃ��Ă��A�K�v�Ȏ��Ɏ����I�ɌĂԂ���
