struct PSOutput
{
	float4 svpos : SV_POSITION;	// ���_�V�F�[�_�[���炫�����W
	float4 color : COLOR;		// ���_�V�F�[�_�[���炫���F
};

float4 BasicPS(PSOutput input) : SV_TARGET
{
	return input.color; // �F�����̂܂ܕ\������
}