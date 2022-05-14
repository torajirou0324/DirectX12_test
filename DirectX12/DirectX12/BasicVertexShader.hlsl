#include "BasicShaderHeader.hlsli"
//Texture2D<float4> tex : register(t0); // 0番スロットに設定されたテクスチャ
//
//SamplerState smp : register(s0); // 0番スロットに設定されたサンプルラー

cbuffer cbuff0 : register(b0) // 定数バッファー
{
	matrix mat; // 変換行列
}

Output BasicVS(float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint2 weight : WEIGHT)
{
	Output output; // ピクセルシェーダーに渡す値
	output.svpos = mul(mat, pos);
	output.normal = normal;
	output.uv = uv;
	return output;
}
