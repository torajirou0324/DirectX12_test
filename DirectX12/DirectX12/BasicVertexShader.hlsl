#include "BasicShaderHeader.hlsli"
Texture2D<float4> tex : register(t0); // 0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); // 0番スロットに設定されたサンプルラー

cbuffer cbuff0 : register(b0) // 定数バッファー
{
	matrix world;    // ワールド変換行列
	matrix viewproj; // ビュープロジェクション行列
}

Output BasicVS(float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint2 weight : WEIGHT)
{
	// ピクセルシェーダーに渡す値
	Output output;
	// シェーダーでは列優先
	output.svpos = mul(mul(viewproj, world), pos);
	normal.w = 0; // 平行移動成分を無効にする
	// 法線にもワールド変換を行う
	output.normal = mul(world, normal);
	output.uv = uv;
	return output;
}
