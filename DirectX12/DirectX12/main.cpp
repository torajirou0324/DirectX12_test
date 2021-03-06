//ポリゴン表示
#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>
#include<map>

#include<d3dcompiler.h>
#include<DirectXTex.h>
#include<d3dx12.h>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

// PMD頂点構造体
struct PMD_VERTEX
{
	XMFLOAT3 pos;        // 頂点座標：12バイト
	XMFLOAT3 normal;     // 法線ベクトル：12バイト
	XMFLOAT2 uv;         // uv座標：8バイト
	uint16_t boneNo[2];  // ボーン番号：4バイト
	uint8_t  boneWeight; // ボーン影響度：1バイト
	uint8_t  edgeFlg;    // 輪郭線フラグ：1バイト
}; // 合計38バイト

///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

//面倒だけど書かなあかんやつ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら呼ばれます
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12Device* _dev = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

///モデルのパスとテクスチャのパスから合成したパスを得る
///@param modelPath　アプリケーションから見たpmdモデルパス
///@param texPath PMDモデルから見たテクスチャのパス
///@return アプリケーションから見たテクスチャのパス
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath)
{
	// (int型に代入しているのは、見つからなかった場合は
	// rfindがepos(-1→0xffffffff)を返すため)
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');

	auto pathIndex = max(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex);
	return folderPath + texPath;
}

///std::string(マルチバイト文字列)からstd::wstring(ワイド文字列)を得る
///@param str マルチバイト文字列
///@return 変換されたワイド文字列
std::wstring GetWideStringFromString(const std::string& str)
{
	// 呼び出し1回目（文字列数を得る）
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr; // stringのwchar_t版
	wstr.resize(num1); // 得られた文字列数でリサイズ

	// 呼び出し2回目(確保済みのwstrに変換文字列をコピー)
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1);

	assert(num1 == num2); // 一応チェック
	return wstr;
}

// ファイル名から拡張子を取得する
//@param path 対象のパス文字列
// @return 拡張子
std::string GetExtension(const std::string& path)
{
	int idx = path.rfind('.');
	return path.substr(
		idx + 1, path.length() - idx - 1);
}

// ファイル名から拡張子を取得する（ワイド文字版）
//@param path 対象のパス文字列
// @return 拡張子
std::wstring GetExtension(const std::wstring& path)
{
	int idx = path.rfind('.');
	return path.substr(
		idx + 1, path.length() - idx - 1);
}

// テクスチャのパスをセパレーター文字列で分離する
// @param path 対象のパス文字列
// @param splitter 区切り文字
// @return 分離前後の文字列ペア
std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*')
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(
		idx + 1, path.length() - idx - 1);

	return ret;
}

//ファイル名パスとリソースのマップテーブル
std::map<std::string, ID3D12Resource*> _resourceTable;

ID3D12Resource* LoadTextureFromFile(std::string& texPath)
{
	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end())
	{
		// テーブル内にあったらロードするのではなくマップ内の
		// リソースを返す
		return _resourceTable[texPath];
	}

	// WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto wtexpath = GetWideStringFromString(texPath);
	auto result = LoadFromWICFile(
		wtexpath.c_str(),
		WIC_FLAGS_NONE,
		&metadata,
		scratchImg);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0); // 生データ抽出

	// WriteToSubresourceで転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0; // 単一アダプターのため0
	texHeapProp.VisibleNodeMask = 0; // 単一アダプターのため0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;
	resDesc.Width = metadata.width;   // 幅
	resDesc.Height = metadata.height; // 高さ
	resDesc.DepthOrArraySize = metadata.arraySize;
	resDesc.SampleDesc.Count = 1; // 通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0; // クオリティは最低
	resDesc.MipLevels = metadata.mipLevels;
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // レイアウトは決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 特にフラグなし

	// バッファー作成
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,// 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	result = texbuff->WriteToSubresource(
		0,
		nullptr,        // 全領域へコピー
		img->pixels,    // 元データアドレス
		img->rowPitch,  // 1ラインサイズ
		img->slicePitch // 全サイズ
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	return texbuff;
}

// 白テクスチャ
ID3D12Resource* CreateWhiteTexture()
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	// texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* whiteBuff = nullptr;

	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff); // 全部255で埋める

	// データ転送
	result = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	return whiteBuff;
}

///アライメントに揃えたサイズを返す
///@param size 元のサイズ
///@param alignment アライメントサイズ
///@return アライメントを揃えたサイズ
size_t AlignmentedSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}

void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

#ifdef _DEBUG
int main() {
#else
#include<Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	DebugOutputFormatString("Show window test.");
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//ウィンドウクラス生成＆登録
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	w.lpszClassName = _T("DirectXTest");//アプリケーションクラス名(適当でいいです)
	w.hInstance = GetModuleHandle(0);//ハンドルの取得
	RegisterClassEx(&w);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	RECT wrc = { 0,0, window_width, window_height };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,//クラス名指定
		_T("DX12 test"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT,//表示X座標はOSにお任せします
		CW_USEDEFAULT,//表示Y座標はOSにお任せします
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		w.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ

#ifdef _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif
	//DirectX12まわり初期化
	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}

	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto l : levels) {
		if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(&_dev)) == S_OK) {
			featureLevel = l;
			break;
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウトなし
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//プライオリティ特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//ここはコマンドリストと合わせてください
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));//コマンドキュー生成

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなので当然RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	// SRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ガンマ補正あり(SRGB)
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (size_t i = 0; i < swcDesc.BufferCount; ++i)
	{
		//ID3D12Resource* backBuffer = nullptr;
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, rtvH);
		rtvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// 深度バッファーの作成
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元のテクスチャデータ
	depthResDesc.Width = window_width; // 幅と高さはレンダーターゲットと同じ
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1; // テクスチャ配列でも、3Dテクスチャでもない
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値書き込み用フォーマット
	depthResDesc.SampleDesc.Count = 1; // サンプルは1ピクセルあたり1つ
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // デプスステンシルとして使用

	// 深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT; // DEFAULTなのであとはUNKNOWNでよい
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// このクリアバッファビューが重要な意味を持つ
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f; // 深さ
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT; // 32ビットfloatとしてクリア

	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,// 深度書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer));

	// 深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1; // 深度ビューは1つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // ステンシルビューをして使う

	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	// 深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // フラグは特になし
	_dev->CreateDepthStencilView(depthBuffer, &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);//ウィンドウ表示

	auto whiteTex = CreateWhiteTexture();

	// PMDヘッダー構造体
	struct PMDHeader
	{
		float version;
		char model_name[20];
		char comment[256];
	};
	char signature[3];
	PMDHeader pmdheader = {};
	std::string strModelPath = "Model/初音ミク.pmd";
	FILE* fp;
	fopen_s(&fp, strModelPath.c_str(), "rb");

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	unsigned int vertNum; // 頂点数
	fread(&vertNum, sizeof(vertNum), 1, fp);

#pragma pack(1) // ここから1バイトパッキングとなり、アライメントは発生しない
	// PMDマテリアル構造体
	struct PMDMaterial
	{
		XMFLOAT3 diffuse;       // ディフーズ色
		float    alpha;         // ディフーズα
		float    specularity;   // スペキュラの強さ（乗算値）
		XMFLOAT3 specular;      // スペキュラ色
		XMFLOAT3 ambient;       // アンビエント色
		unsigned char toonIdx;  // トゥーン番号（後述）
		unsigned char edgeFlg;  // マテリアルごとの輪郭線フラグ

		// ここに2バイトのパディングがある
		unsigned int indicesNum;// このマテリアルが割り当てられるインデックス数

		char texFilePath[20];   // テクスチャファイルパス＋α（後述）
	}; // 70バイトのはずだが、パディングが発生するため72バイト
#pragma pack() // パッキング指定を解除（デフォルトに戻す）

	// シェーダー側に投げられるマテリルデータ
	struct MaterialForHlsl
	{
		XMFLOAT3 diffuse;      // ディフューズ色
		float    alpha;        // ディフューズα
		XMFLOAT3 specular;     // スペキュラ色
		float    specularity;  // スペキュラの強さ（乗算値）
		XMFLOAT3 ambient;      // アンビエント
	};

	// それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		std::string texPath; // テクスチャファイルパス
		int         toonIdx; // トゥーン番号
		bool        edgeFlg; // マテリアルごとの輪郭線フラグ
	};

	// 全体をまとめるデータ
	struct Material
	{
		unsigned int       indicesNum;  // インデックス数
		MaterialForHlsl    material;
		AdditionalMaterial additional;
	};

	constexpr size_t pmdvertex_size = 38; // 頂点1つあたりのサイズ
	std::vector<PMD_VERTEX>vertices(vertNum); // バッファの確保
	for (auto i = 0; i < vertNum; i++)
	{
		fread(&vertices[i], pmdvertex_size, 1, fp);
	}

	unsigned int indicesNum; // インデックス数
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMD_VERTEX));
	//UPLOAD(確保は可能)
	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	PMD_VERTEX* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファの仮想アドレス
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX)); //全バイト数
	vbView.StrideInBytes = sizeof(PMD_VERTEX);//1頂点あたりのバイト数

	std::vector<unsigned short> indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp); // 一気に読み込む

	unsigned int materialNum; // マテリアル数
	fread(&materialNum, sizeof(materialNum), 1, fp);

	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp); // 一気に読み込む
	std::vector<Material> materials(pmdMaterials.size());

	std::vector<ID3D12Resource*> textureResource(materialNum);


	// コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
	}
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			textureResource[i] = nullptr;
			continue;
		}
		std::string texFileName = pmdMaterials[i].texFilePath;
		if (std::count(texFileName.begin(), texFileName.end(), ('*') > 0))
		{
			// スプリッタがある
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph" ||
				GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
			}
			else
			{
				texFileName = namepair.first;
			}
		}

		// モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
		auto texFilePath = GetTexturePathFromModelAndTexPath(
			strModelPath,
			texFileName.c_str());

		textureResource[i] = LoadTextureFromFile(texFilePath);
	}

	fclose(fp);

	ID3D12Resource* idxBuff = nullptr;
	resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
	//設定は、バッファのサイズ以外頂点バッファの設定を使いまわして
	//OKだと思います。
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	//作ったバッファにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//インデックスバッファビューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	// マテリアルバッファを作成
	auto materialBufferSize = sizeof(MaterialForHlsl);
	materialBufferSize = (materialBufferSize + 0xff) & ~0xff;
	// もったいないが仕方ない
	auto MresDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBufferSize * materialNum);

	ID3D12Resource* materialBuff = nullptr; // マテリルバッファのマクロ

	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&MresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff)
	);

	// マップマテリアルコピー
	char* mapMaterial = nullptr;

	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto& m : materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; // データコピー
		mapMaterial += materialBufferSize; // 次のアライメント位置まで進める（256の倍数）
	}
	materialBuff->Unmap(0, nullptr);

	ID3D12DescriptorHeap* materialDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum * 2; // マテリアル数×2
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&matDescHeapDesc, IID_PPV_ARGS(&materialDescHeap));

	// ビューの作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress(); // バッファーアドレス
	matCBVDesc.SizeInBytes = materialBufferSize; // 256のアライメントサイズ
	// 通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // デフォルト
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 後術
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1; // ミップマップは使用しないので1

	auto matDescHeapH = materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// ループの中でビューを作成していく
	for (int i = 0; i < materialNum; ++i)
	{
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += incSize;
		matCBVDesc.BufferLocation += materialBufferSize;
		// シェーダーリソースビュー
		if (textureResource[i] == nullptr)
		{
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex, &srvDesc, matDescHeapH);
		}
		else
		{
			srvDesc.Format = textureResource[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				textureResource[i], &srvDesc, matDescHeapH);
		}

		matDescHeapH.ptr += incSize;
	}

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* errorBlob = nullptr;
	result = D3DCompileFromFile(L"BasicVertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_vsBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
	}
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		// 座標情報
		{ "POSITION",
		0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		// 法線ベクトル
		{"NORMAL",
		0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// uv（追加）
		{ "TEXCOORD",0,
		DXGI_FORMAT_R32G32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// ボーン番号
		{"BONE_NO",
		0,DXGI_FORMAT_R16G16_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// ウェイト
		{"WEIGHT",
		0,DXGI_FORMAT_R8_UINT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// 輪郭線フラグ
		//{"EDGE_FLG",
	 //   0,DXGI_FORMAT_R8_UINT,0,
	 //   D3D12_APPEND_ALIGNED_ELEMENT,
	 //   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff

	//
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	//ひとまず加算や乗算やαブレンディングは使用しない
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//ひとまず論理演算は使用しない
	renderTargetBlendDesc.LogicOpEnable = false;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;


	gpipeline.RasterizerState.MultisampleEnable = false;//まだアンチェリは使わない
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

	//残り
	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


	gpipeline.DepthStencilState.DepthEnable = true; // 深度バッファを使う
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 書き込む
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // 小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 32ビットfloatを深度値として使用する
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline.NumRenderTargets = 1;//今は１つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0〜1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低

	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// ディスクリプタレンジ
	D3D12_DESCRIPTOR_RANGE descTblRange[3] = {}; // テクスチャと定数の2つ
	// 定数1つ目(座標変換用)
	descTblRange[0].NumDescriptors = 1;
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // 種別は定数
	descTblRange[0].BaseShaderRegister = 0; // 0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 定数2つ目(マテリアル用)
	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // 種別は定数
	descTblRange[1].BaseShaderRegister = 1; // 1番スロットから
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// テクスチャ1つ目(マテリアルとペア)
	descTblRange[2].NumDescriptors = 1; // テクスチャ1つ
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // 種別はテクスチャ
	descTblRange[2].BaseShaderRegister = 0; // 0番スロットから
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータの設定
	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];	// 配列先アドレス
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;						// ディスクリプタレンジ数
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;		// すべてのシェーダーから見える

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];		// ディスクリプタレンジのアドレス
	rootparam[1].DescriptorTable.NumDescriptorRanges = 2;							// ディスクリプタレンジ数
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;			// すべてのシェーダーから見える

	rootSignatureDesc.pParameters = &rootparam[0];									// ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 2;

	D3D12_STATIC_SAMPLER_DESC sampleDesc = {};
	sampleDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 横方向の繰り返し
	sampleDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 縦方向の繰り返し
	sampleDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 奥行きの繰り返し
	sampleDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; // ボーダーは黒
	sampleDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 線形補間D
	sampleDesc.MaxLOD = D3D12_FLOAT32_MAX; // ミップマップ最大値
	sampleDesc.MinLOD = 0.0f;              // ミップマップ最小値
	sampleDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーから見える
	sampleDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;     // リサンプリングしない

	rootSignatureDesc.pStaticSamplers = &sampleDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();

	gpipeline.pRootSignature = rootsignature;
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;//出力先の幅(ピクセル数)
	viewport.Height = window_height;//出力先の高さ(ピクセル数)
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値


	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;//切り抜き上座標
	scissorrect.left = 0;//切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;//切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;//切り抜き下座標

	struct MatricesData
	{
		XMMATRIX world;    // モデル本体を回転させたり移動させたりする行列
		XMMATRIX viewproj; // ビューとプロジェクション合成行列
	};

	// 定数バッファー作成
	auto worldMat = XMMatrixRotationY(XM_PIDIV4);
	XMFLOAT3 eye(0.0f, 10.0f, -15.0f);   // 視点
	XMFLOAT3 target(0.0f, 10.0f, 0.0f); // 注視点
	XMFLOAT3 up(0.0f, 1.0f, 0.0f);     // 上ベクトル
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2, // 画角は90°
		static_cast<float>(window_width) / static_cast<float>(window_height), // アスペクト比
		1.0f, // 近いほう
		100.0f // 遠いほう
	);
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);
	ID3D12Resource* constBuff = nullptr;
	_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);

	MatricesData* mapMatrix; // マップ先を示すポインター
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix); // マップ（なにかとなにかを関連付ける）
	// 行列の内容をコピー
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;

	// ディスクリプタヒープを作る
	ID3D12DescriptorHeap* basicDescHeap = nullptr; // 基本的な情報の受け渡し用
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーが見えるように
	descHeapDesc.NodeMask = 0; // マスクは0
	descHeapDesc.NumDescriptors = 1;// SRVとCBVで2つ
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // ディスクリプタヒープ種別
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap)); // 生成

		// ディスクリプタの先頭ハンドルを取得しておく
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
	// 定数バッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

	MSG msg = {};
	unsigned int frame = 0;
	float angle = 0.0f;
	while (true)
	{
		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;
		mapMatrix->viewproj = viewMat * projMat;
		angle += 0.02f;

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//もうアプリケーションが終わるって時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}

		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		_cmdList->SetPipelineState(_pipelinestate);

		//レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_cmdList->ResourceBarrier(1, &barrier);
		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

		//画面クリア
		float clearColor[] = { 1.0f,1.0f,1.0f,1.0f }; // 青色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);

		// 描画時の設定・・・ルートシグネチャの指定。ディスクリプタヒープの指定。
		// ルートパラメータインデックスとディスクリプタヒープのアドレスの関連付け
		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		// ルートパラメーターとディスクリプタヒープのバインド設定
		_cmdList->SetGraphicsRootDescriptorTable(0, // ルートパラメータインデックス
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()); // ヒープアドレス

		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);

		auto materialH = materialDescHeap->GetGPUDescriptorHandleForHeapStart(); // ヒープの先頭
		auto cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 2; // 2倍
		unsigned int idxOffset = 0; // 最初はオフセットなし
		for (auto& m : materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1,
				materialH);
			_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

			// ヒープポインターとインデックスを次に進める
			materialH.ptr += cbvsrvIncSize; // 次のビューのために2倍進める
			idxOffset += m.indicesNum;
		}
		_cmdList->SetGraphicsRootDescriptorTable(1,
			materialDescHeap->GetGPUDescriptorHandleForHeapStart());

		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		barrier = CD3DX12_RESOURCE_BARRIER::Transition(_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		_cmdList->ResourceBarrier(1, &barrier);

		//命令のクローズ
		_cmdList->Close();

		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		////待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, _pipelinestate);//再びコマンドリストをためる準備


		//フリップ
		_swapchain->Present(1, 0);

	}
	//もうクラス使わんから登録解除してや
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}