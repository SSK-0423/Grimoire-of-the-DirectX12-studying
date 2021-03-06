#include "Application.h"
#include"Dx12Wrapper.h"
#include"PMDRenderer.h"
#include"PMDActor.h"

//ウィンドウ定数
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

//面倒だけど書かなあかんやつ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら呼ばれます
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
}

void 
Application::CreateGameWindow(HWND &hwnd, WNDCLASSEX &windowClass) {
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//ウィンドウクラス生成＆登録
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	windowClass.lpszClassName = _T("DirectXTest");//アプリケーションクラス名(適当でいいです)
	windowClass.hInstance = GetModuleHandle(0);//ハンドルの取得
	RegisterClassEx(&windowClass);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	RECT wrc = { 0,0, window_width, window_height };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
	//ウィンドウオブジェクトの生成
	hwnd = CreateWindow(windowClass.lpszClassName,//クラス名指定
		_T("DX12アニメーション"),//タイトルバーの文字
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

SIZE
Application::GetWindowSize()const {
	SIZE ret;
	ret.cx = window_width;
	ret.cy = window_height;
	return ret;
}

void 
Application::Run() {
	ShowWindow(_hwnd, SW_SHOW);//ウィンドウ表示
	float angle = 0.0f;
	MSG msg = {};
	unsigned int frame = 0;
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//もうアプリケーションが終わるって時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}

		//シャドウマップ
		//1.ライト視点での深度値撮影	shadowPipeline rootsignature
		//2.モデル通常描画	
		// 
		// peraPipeline peraRootsignature
		//3.深度値比較して影をつける

		_dx12->SetCameraSetting();
		_pmdRenderer->BeforeDrawFromLight();	//shadowPipeline rootsignature
		
		//影描画
		{
			_dx12->PreDrawShadow();
			_dx12->SetScene();
			for (auto& pmd : _pmdActors)
			{
				pmd->Update();
				pmd->Draw(true);
			}
			_dx12->EndDrawShadow();	//今は何もしない
		}
		//通常描画
		//ここでシャドウマップをかける
		{
			_pmdRenderer->BeforeDraw();	// pipeline rootsignature
			_dx12->PreDrawRenderTarget1();
			_dx12->SetScene();
			for (auto& pmd : _pmdActors)
			{
				pmd->Draw(false);
			}
			_dx12->EndDrawRenderTarget1();
			_dx12->DrawShrinkTextureForBlur();
		}
		//2パス目の結果をテクスチャとして受け取って表示
		{
			//ここは多分あってる
			_dx12->PreDrawFinalRenderTarget();
			_dx12->DrawFinalRenderTarget();
			_dx12->EndDraw();
		}
		//フリップ
		_dx12->Swapchain()->Present(1, 0);
	}
}

bool 
Application::Init() {
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(_hwnd, _windowClass);

	//DirectX12ラッパー生成＆初期化
	_dx12.reset(new Dx12Wrapper(_hwnd));
	_pmdRenderer.reset(new PMDRenderer(*_dx12));

	_pmdActor.reset(new PMDActor("Model/鏡音リン.pmd", *_pmdRenderer));
	_pmdActor->Move(0, 0, 0);
	_pmdActor2.reset(new PMDActor("Model/初音ミク.pmd", *_pmdRenderer));
	_pmdActor2->Move(10, 0, 10);

	_pmdActors.resize(3);
	_pmdActors[0].reset(new PMDActor("Model/初音ミク.pmd", *_pmdRenderer));
	_pmdActors[0]->Move(0, 0, 0);
	_pmdActors[1].reset(new PMDActor("Model/鏡音リン.pmd", *_pmdRenderer));
	_pmdActors[1]->Move(10, 0, 10);
	_pmdActors[2].reset(new PMDActor("Model/巡音ルカ.pmd", *_pmdRenderer));
	_pmdActors[2]->Move(-10, 0, 10);
	//_pmdActor->PlayAnimation();
	return true;
}

void
Application::Terminate() {
	//もうクラス使わんから登録解除してや
	UnregisterClass(_windowClass.lpszClassName, _windowClass.hInstance);
}

Application& 
Application::Instance() {
	static Application instance;
	return instance;
}

Application::Application()
{
}


Application::~Application()
{
}