// 定数バッファー
// マテリアル用
cbuffer Material : register(b1)
{
	float4 diffuse;		// ディフューズ色
	float4 specular;	// スペキュラ
	float3 ambient;	// アンビエント
}

// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float4 normal : NORMAL;     // 法線ベクトル
	float2 uv : TEXCOORD;       // uv値
};

//ConstantBuffer<Matrix> m : register(b0);