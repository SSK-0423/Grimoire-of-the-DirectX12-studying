#include "Application.h"
#include"Dx12Wrapper.h"
#include"PMDRenderer.h"
#include"PMDActor.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

//�E�B���h�E�萔
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

//���̊֐��̖{�̂�imgui_win32_impl.cpp�ɂ���̂�extern�Ńv���g�^�C�v�錾����
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

//�ʓ|�����Ǐ����Ȃ�������
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//�E�B���h�E���j�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);//OS�ɑ΂��āu�������̃A�v���͏I�����v�Ɠ`����
		return 0;
	}
	ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
	return DefWindowProc(hwnd, msg, wparam, lparam);//�K��̏������s��
}

void 
Application::CreateGameWindow(HWND &hwnd, WNDCLASSEX &windowClass) {
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//�E�B���h�E�N���X�������o�^
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
	windowClass.lpszClassName = _T("DirectXTest");//�A�v���P�[�V�����N���X��(�K���ł����ł�)
	windowClass.hInstance = GetModuleHandle(0);//�n���h���̎擾
	RegisterClassEx(&windowClass);//�A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

	RECT wrc = { 0,0, window_width, window_height };//�E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����
	//�E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(windowClass.lpszClassName,//�N���X���w��
		_T("DX12�A�j���[�V����"),//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT,//�\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,//�\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		windowClass.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^

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
	ShowWindow(_hwnd, SW_SHOW);//�E�B���h�E�\��
	float angle = 0.0f;
	MSG msg = {};
	unsigned int frame = 0;
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//�����A�v���P�[�V�������I�����Ď���message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT) {
			break;
		}
		//�V���h�E�}�b�v
		//1.���C�g���_�ł̐[�x�l�B�e	shadowPipeline rootsignature
		//2.���f���ʏ�`��	
		// 
		// peraPipeline peraRootsignature
		//3.�[�x�l��r���ĉe������

		_dx12->SetCameraSetting();
		_pmdRenderer->BeforeDrawFromLight();	//shadowPipeline rootsignature
		
		//�e�`��
		{
			_dx12->PreDrawShadow();
			_dx12->SetScene();
			for (auto& pmd : _pmdActors)
			{
				pmd->Update();
				pmd->Draw(true);
			}
			_dx12->EndDrawShadow();	//���͉������Ȃ�
		}
		//�ʏ�`��
		//�����ŃV���h�E�}�b�v��������
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
		//�A���r�G���g�I�N���[�W�����`��
		{
			_dx12->DrawAmbientOcculusion();
		}
		//2�p�X�ڂ̌��ʂ��e�N�X�`���Ƃ��Ď󂯎���ĕ\��
		{
			//�����͑��������Ă�
			_dx12->PreDrawFinalRenderTarget();
			_dx12->DrawFinalRenderTarget();
		}

		//ImGui�`��
		//ExcuteCommand�Ȃǂ̑O�ɋL�q����
		//���f���Ȃǂ��O�ɕb������ƁA�h��Ԃ���Ă��܂��̂�
		//GUI�̕`��͍Ō�ɍs�������悢
		{
			//�`��O������
			//���̏����͎d�l
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			{
				ImGui::Begin("Rendering Test Menu"); //�E�B���h�E�̖��O
				ImGui::SetWindowSize(
					ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);//�E�B���h�E�T�C�Y
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

				// Dx12Wrapper�ɑ΂��Đݒ��n��
				_dx12->SetDebugDisplay(blnDebugDsip);
				_dx12->SetSSAO(blnSSAO);
				_dx12->SetSelfShadow(blnShadowmap);
				_dx12->SetFov(fov);
				_dx12->SetLightVector(lightVec);
				_dx12->SetBackColor(bgcol4);
				_dx12->SetBloomColor(bloomCol);

				ImGui::End();
			}
			
			//�ȉ���3�s���������Ƃŕ`�悳���
			ImGui::Render();	
			_dx12->CommandList()->SetDescriptorHeaps(
				1, _dx12->GetHeapForImgui().GetAddressOf());
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _dx12->CommandList().Get());
		}

		_dx12->EndDraw();
		//�t���b�v
		_dx12->Swapchain()->Present(1, 0);
	}
}

bool 
Application::Init() {
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(_hwnd, _windowClass);

	//DirectX12���b�p�[������������
	_dx12.reset(new Dx12Wrapper(_hwnd));
	_pmdRenderer.reset(new PMDRenderer(*_dx12));

	_pmdActor.reset(new PMDActor("Model/��������.pmd", *_pmdRenderer));
	_pmdActor->Move(0, 0, 0);
	_pmdActor2.reset(new PMDActor("Model/�����~�N.pmd", *_pmdRenderer));
	_pmdActor2->Move(10, 0, 10);

	_pmdActors.resize(3);
	_pmdActors[0].reset(new PMDActor("Model/�����~�N.pmd", *_pmdRenderer));
	_pmdActors[0]->Move(0, 0, 0);
	_pmdActors[1].reset(new PMDActor("Model/��������.pmd", *_pmdRenderer));
	_pmdActors[1]->Move(10, 0, 10);
	_pmdActors[2].reset(new PMDActor("Model/�������J.pmd", *_pmdRenderer));
	_pmdActors[2]->Move(-10, 0, 10);
	//_pmdActor->PlayAnimation();

	//ImGui�֘A�̏�����
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
			_dx12->Device().Get(),// DirectX12�f�o�C�X
			3,	//��q�F�����ɂ�frames_in_flight�Ƃ��邪
			DXGI_FORMAT_R8G8B8A8_UNORM,
			_dx12->GetHeapForImgui().Get(),
			_dx12->GetHeapForImgui()->GetCPUDescriptorHandleForHeapStart(),
			_dx12->GetHeapForImgui()->GetGPUDescriptorHandleForHeapStart());
	}
	return true;
}

void
Application::Terminate() {
	//�����N���X�g��񂩂�o�^�������Ă�
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