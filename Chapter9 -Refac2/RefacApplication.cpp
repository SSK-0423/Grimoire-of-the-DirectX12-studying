#include "RefacApplication.h"
#include "Dx12Wrapper.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

//ウィンドウ定数
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

//面倒だけど書かなあかんやつ
//ウィンドウクラスのコールバック関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{	//ウィンドウが破棄されたら呼ばれます
		PostQuitMessage(0);//OSに対してアプリケーションの終了を伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
}

//ウィンドウ生成
void RefacApplication::CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass)
{
	//以下を書いておかないとCOMが旨く動かずWICが正常に動作しないことがあります。
	//(書かなくても動くときもあります)
	//result = CoInitializeEx(0, COINIT_MULTITHREADED);
	//DebugOutputFormatString("Show window test.");
	HINSTANCE hInst = GetModuleHandle(nullptr);

	//ウィンドウクラス生成＆登録
	windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	windowClass.lpszClassName = _T("Chapter9-Refactor");//アプリケーションクラス名(適当でいいです)
	windowClass.hInstance = GetModuleHandle(0);//ハンドルの取得
	RegisterClassEx(&windowClass);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	//ウィンドウサイズを決める
	RECT wrc = { 0,0, window_width, window_height };

	//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウオブジェクトの生成
	hwnd = CreateWindow(windowClass.lpszClassName,//クラス名指定
		_T("Chapter9-Refactor"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT,//表示X座標はOSにお任せします
		CW_USEDEFAULT,//表示Y座標はOSにお任せします
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		windowClass.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ
}

//ウィンドウサイズ取得
SIZE RefacApplication::GetWindowSize() const
{
	SIZE ret;
	ret.cx = window_width;
	ret.cy = window_height;
	return ret;
}

//初期化
bool RefacApplication::Init()
{
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(_hwnd, _windowClass);

	//DirectX12ラッパー生成＆初期化
	_dx12.reset(new Dx12Wrapper(_hwnd));	//Dx12Wrapperポインタを_dx12に管理させる

	_pmdRenderer.reset(new PMDRenderer(*_dx12));
	_pmdActor.reset(new PMDActor("Model/初音ミクmetal.pmd", *_pmdRenderer,*_dx12));

	_pmdActors.resize(3);

	_pmdActors[0].reset(new PMDActor("Model/鏡音リン.pmd", *_pmdRenderer, *_dx12));
	_pmdActors[1].reset(new PMDActor("Model/巡音ルカ.pmd", *_pmdRenderer, *_dx12));
	_pmdActors[2].reset(new PMDActor("Model/弱音ハク.pmd", *_pmdRenderer, *_dx12));

	return true;
}

//実行
void RefacApplication::Run()
{
	ShowWindow(_hwnd, SW_SHOW);
	float angle = 0.f;
	MSG msg = {};
	unsigned int frame = 0;

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//アプリケーション終了時
		if (msg.message == WM_QUIT)
			break;

		//全体の描画準備
		_dx12->BeginDraw();

		//PMD用の描画パイプラインに合わせる
		_dx12->CommandList()->SetPipelineState(_pmdRenderer->GetPipelineState());

		//ルートシグネチャもPMD用に合わせる
		_dx12->CommandList()->SetGraphicsRootSignature(_pmdRenderer->GetRootSignature());

		_dx12->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_dx12->SetScene();

		_pmdActor->Update();
		_pmdActor->Draw();

		for (const auto& actor : _pmdActors)
		{
			actor->Update();
			actor->Draw();
		}

		_dx12->EndDraw();

		//フリップ
		_dx12->Swapchain()->Present(1, 0);
	}
}

//終了
void RefacApplication::Terminate()
{
	//クラスの登録解除
	UnregisterClass(_windowClass.lpszClassName, _windowClass.hInstance);
}


//シングルトンインスタンス取得
RefacApplication& RefacApplication::Instance()
{
	static RefacApplication instance;
	return instance;
}
