#include "BasicShaderHeader.hlsli"

Output BasicVS(float4 svpos : POSITION, float2 uv : TEXCOORD)
{
	Output output; // �s�N�Z���V�F�[�_�[�ɓn���l
	output.svpos = mul(mat, svpos);
	output.uv = uv;
	return output;
}
