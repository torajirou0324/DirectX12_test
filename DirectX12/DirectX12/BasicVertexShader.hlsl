#include "BasicShaderHeader.hlsli"

Output BasicVS(float4 svpos : POSITION, float2 uv : TEXCOORD)
{
	Output output; // �s�N�Z���V�F�[�_�[�ɓn���l
	output.svpos = svpos;
	output.uv = uv;
	return output;
}
