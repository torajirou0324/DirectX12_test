#include "BasicShaderHeader.hlsli"
//Texture2D<float4> tex : register(t0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
//
//SamplerState smp : register(s0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v�����[

cbuffer cbuff0 : register(b0) // �萔�o�b�t�@�[
{
	matrix mat; // �ϊ��s��
}

Output BasicVS(float4 svpos : POSITION, float2 uv : TEXCOORD)
{
	Output output; // �s�N�Z���V�F�[�_�[�ɓn���l
	output.svpos = mul(mat, svpos);
	output.uv = uv;
	return output;
}
