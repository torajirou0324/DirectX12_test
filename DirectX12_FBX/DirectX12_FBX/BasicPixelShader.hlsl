struct PSOutput
{
	float4 svpos : SV_POSITION;	// 頂点シェーダーからきた座標
	float4 color : COLOR;		// 頂点シェーダーからきた色
};

float4 BasicPS(PSOutput input) : SV_TARGET
{
	return input.color; // 色をそのまま表示する
}