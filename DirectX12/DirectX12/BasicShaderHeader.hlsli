// �萔�o�b�t�@�[
// �}�e���A���p
cbuffer Material : register(b1)
{
	float4 diffuse;		// �f�B�t���[�Y�F
	float4 specular;	// �X�y�L����
	float3 ambient;	// �A���r�G���g
}

// ���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos : SV_POSITION; // �V�X�e���p���_���W
	float4 normal : NORMAL;     // �@���x�N�g��
	float2 uv : TEXCOORD;       // uv�l
};

//ConstantBuffer<Matrix> m : register(b0);