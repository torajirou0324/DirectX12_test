#include "BasicShaderHeader.hlsli"
//Texture2D<float4> tex : register(t0); // 0番スロットに設定されたテクスチャ
//
//SamplerState smp : register(s0); // 0番スロットに設定されたサンプルラー

cbuffer cbuff0 : register(b0) // 定数バッファー
{
	matrix mat; // 変換行列
}

Output BasicVS(float4 svpos : POSITION, float2 uv : TEXCOORD)
{
	Output output; // ピクセルシェーダーに渡す値
	output.svpos = mul(mat, svpos);
	output.uv = uv;
	return output;
}
