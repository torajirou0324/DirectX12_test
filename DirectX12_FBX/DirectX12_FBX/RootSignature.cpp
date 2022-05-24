#include "RootSignature.h"
#include "Engine.h"
#include <d3dx12.h>

RootSignature::RootSignature()
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	// アプリケーションの入力アセンブラを使用する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;			// ドメインシェーダーのルートシグネチャへんアクセスを拒否する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;				// ハルシェーダーのルートシグネチャへんアクセスを拒否する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;			// ジオメトリシェーダーのルートシグネチャへんアクセスを拒否する

	CD3DX12_ROOT_PARAMETER rootParam[1] = {};
	rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // b0の定数バッファを設定、全てのシェーダーから見えるようにする

	// スタティックサンプラーの設定
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	// ルートシグネチャの設定(設定したいルートパラメーターとスタティックサンプラーを入れる)
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = std::size(rootParam); // ルートパラメータの個数を入れる
	desc.NumStaticSamplers = 1; // サンプラーの個数を入れる
	desc.pParameters = rootParam; // ルートパラメーターのポインタを入れる
	desc.pStaticSamplers = &sampler; // サンプラーのポインタを入れる
	desc.Flags = flag; // フラグを設定

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	// シリアライズ
	auto hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("ルートシグネチャシリアライズに失敗\n");
		return;
	}
	else
	{
		printf("ルートシグネチャのシリアライズに成功\n");
	}

	// ルートシグネチャ生成
	hr = g_Engine->Device()->CreateRootSignature(
		0, // GPUが複数ある場合のノードマスク（今回は１個しかない想定なので0）
		pBlob->GetBufferPointer(), // シリアライズしたデータのポインタ
		pBlob->GetBufferSize(),    // シリアライズしたデータのサイズ
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		printf("ルートシグネチャの生成に失敗\n");
		return;
	}
	else
	{
		printf("ルートシグネチャの生成に成功\n");
	}

	m_IsValid = true;
}

bool RootSignature::IsValid()
{
	return m_IsValid;
}

ID3D12RootSignature* RootSignature::Get()
{
	return m_pRootSignature.Get();
}
