#pragma once
#include "ComPtr.h"

// ルートシグネチャ
// シェーダー内で使用する定数バッファやテクスチャ、サンプラーなどのリソースのレイアウト（メモリ上にどう並んでいるか）を定義するオブジェクト
// ルートシグネチャを使って、シェーダー内のどのレジスターとGPUのどのメモリの内容を紐づけるかを決定する
// シェーダーにいろいろなデータを渡したい場合、ルートシグネチャをちゃんと使いこなす必要がある

struct ID3D12RootSignature;

class RootSignature
{
public:
	RootSignature(); // コンストラクタでルートシグネチャを生成
	bool IsValid();  // ルートシグネチャの生成に失敗したかどうかを返す
	ID3D12RootSignature* Get(); // ルートシグネチャを返す

private:
	bool m_IsValid = false; // ルートシグネチャの生成に成功したかどうか
	ComPtr<ID3D12RootSignature> m_pRootSignature = nullptr; // ルートシグネチャ
};