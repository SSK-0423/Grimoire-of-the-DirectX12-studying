#include "Application.h"
#include"Dx12Wrapper.h"
#include"PMDRenderer.h"
#include"PMDActor.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

//ウィンドウ定数
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

//この関数の本体がimgui_win32_impl.cppにあるのでexternでプロトタイプ宣言する
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

//面倒だけど書かなあかんやつ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら呼ばれます
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
		return 0;
	}
	ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
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
		//アンビエントオクルージョン描画
		{
			_dx12->DrawAmbientOcculusion();
		}
		//2パス目の結果をテクスチャとして受け取って表示
		{
			//ここは多分あってる
			_dx12->PreDrawFinalRenderTarget();
			_dx12->DrawFinalRenderTarget();
		}

		//ImGui描画
		//ExcuteCommandなどの前に記述する
		//モデルなどより前に秒がすると、塗りつぶされてしまうので
		//GUIの描画は最後に行う方がよい
		{
			//描画前初期化
			//この順序は仕様
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			{
				ImGui::Begin("Rendering Test Menu"); //ウィンドウの名前
				ImGui::SetWindowSize(
					ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);//ウィンドウサイズ
				static bool blnDebugDsip = false;
				ImGui::Checkbox("DebugDisplay", &blnDebugDsip);

				static bool blnSSAO = false;
				ImGui::Checkbox("SSAO on/off", &blnSSAO);

				static bool blnShadowmap = false;
				ImGui::Checkbox("Self Shadow on/off", &blnShadowmap);

				constexpr float pi = 3.141592653589f;
				static float fov = pi / 4.f;
				ImGui::SliderFloat("Field of view",&fov, pi / 6.f, pi * 5.f / 6.f);

				static float lightVec[3] = {-1.f,1.f,-1.f};
				ImGui::SliderFloat3("Light vector", lightVec, -1.f, 1.f);

				static float bgcol4[4] = {0.5f,0.5f,0.5f,1.f};
				ImGui::ColorPicker4("BG Color", bgcol4,
					ImGuiColorEditFlags_::ImGuiColorEditFlags_PickerHueWheel |
					ImGuiColorEditFlags_::ImGuiColorEditFlags_AlphaBar);

				static float bloomCol[3] = {};
				ImGui::ColorPicker3("Bloom Color", bloomCol);

				// Dx12Wrapperに対して設定を渡す
				_dx12->SetDebugDisplay(blnDebugDsip);
				_dx12->SetSSAO(blnSSAO);
				_dx12->SetSelfShadow(blnShadowmap);
				_dx12->SetFov(fov);
				_dx12->SetLightVector(lightVec);
				_dx12->SetBackColor(bgcol4);
				_dx12->SetBloomColor(bloomCol);

				ImGui::End();
			}
			
			//以下の3行を書くことで描画される
			ImGui::Render();	
			_dx12->CommandList()->SetDescriptorHeaps(
				1, _dx12->GetHeapForImgui().GetAddressOf());
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _dx12->CommandList().Get());
		}

		_dx12->EndDraw();
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

	//ImGui関連の初期化
	{
		if (ImGui::CreateContext() == nullptr)
		{
			assert(0);
			return false;
		}
		//[ImGui::]
		bool blnResult = ImGui_ImplWin32_Init(_hwnd);
		if (!blnResult)
		{
			assert(0);
			return false;
		}
		blnResult = ImGui_ImplDX12_Init(
			_dx12->Device().Get(),// DirectX12デバイス
			3,	//後述：説明にはframes_in_flightとあるが
			DXGI_FORMAT_R8G8B8A8_UNORM,
			_dx12->GetHeapForImgui().Get(),
			_dx12->GetHeapForImgui()->GetCPUDescriptorHandleForHeapStart(),
			_dx12->GetHeapForImgui()->GetGPUDescriptorHandleForHeapStart());
	}
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