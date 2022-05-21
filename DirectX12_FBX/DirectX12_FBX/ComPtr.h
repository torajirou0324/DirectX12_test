#pragma once
#include <wrl/client.h>

template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

// DirectX12関連のオブジェクトは終了時に解放処理をきちんと行いたいためComPtrを使用する
// いちいちRelease関数を呼ばなくても、必要な時に自動的に呼ぶもの
