#include "BasicShaderHeader.hlsli"
Texture2D<float4> tex : register(t0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp : register(s0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v�����[

cbuffer cbuff0 : register(b0) // �萔�o�b�t�@�[
{
	matrix world;    // ���[���h�ϊ��s��
	matrix viewproj; // �r���[�v���W�F�N�V�����s��
}

Output BasicVS(float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint2 weight : WEIGHT)
{
	// �s�N�Z���V�F�[�_�[�ɓn���l
	Output output;
	// �V�F�[�_�[�ł͗�D��
	output.svpos = mul(mul(viewproj, world), pos);
	normal.w = 0; // ���s�ړ������𖳌��ɂ���
	// �@���ɂ����[���h�ϊ����s��
	output.normal = mul(world, normal);
	output.uv = uv;
	return output;
}
