#include "BasicShaderHeader.hlsli"

Output BasicVS(float4 svpos : POSITION, float2 uv : TEXCOORD)
{
	Output output; // ピクセルシェーダーに渡す値
	output.svpos = mul(mat, svpos);
	output.uv = uv;
	return output;
}
