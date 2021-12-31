#include "RefacApplication.h"
#include "Dx12Wrapper.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

//�E�B���h�E�萔
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

//�ʓ|�����Ǐ����Ȃ�������
//�E�B���h�E�N���X�̃R�[���o�b�N�֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{	//�E�B���h�E���j�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);//OS�ɑ΂��ăA�v���P�[�V�����̏I����`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//�K��̏������s��
}

//�E�B���h�E����
void RefacApplication::CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass)
{
	//�ȉ��������Ă����Ȃ���COM���|��������WIC������ɓ��삵�Ȃ����Ƃ�����܂��B
	//(�����Ȃ��Ă������Ƃ�������܂�)
	//result = CoInitializeEx(0, COINIT_MULTITHREADED);
	//DebugOutputFormatString("Show window test.");
	HINSTANCE hInst = GetModuleHandle(nullptr);

	//�E�B���h�E�N���X�������o�^
	windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
	windowClass.lpszClassName = _T("Chapter9-Refactor");//�A�v���P�[�V�����N���X��(�K���ł����ł�)
	windowClass.hInstance = GetModuleHandle(0);//�n���h���̎擾
	RegisterClassEx(&windowClass);//�A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

	//�E�B���h�E�T�C�Y�����߂�
	RECT wrc = { 0,0, window_width, window_height };

	//�E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//�E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(windowClass.lpszClassName,//�N���X���w��
		_T("Chapter9-Refactor"),//�^�C�g���o�[�̕���
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

//�E�B���h�E�T�C�Y�擾
SIZE RefacApplication::GetWindowSize() const
{
	SIZE ret;
	ret.cx = window_width;
	ret.cy = window_height;
	return ret;
}

//������
bool RefacApplication::Init()
{
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(_hwnd, _windowClass);

	//DirectX12���b�p�[������������
	_dx12.reset(new Dx12Wrapper(_hwnd));	//Dx12Wrapper�|�C���^��_dx12�ɊǗ�������

	_pmdRenderer.reset(new PMDRenderer(*_dx12));
	_pmdActor.reset(new PMDActor("Model/�����~�Nmetal.pmd", *_pmdRenderer,*_dx12));

	_pmdActors.resize(3);

	_pmdActors[0].reset(new PMDActor("Model/��������.pmd", *_pmdRenderer, *_dx12));
	_pmdActors[1].reset(new PMDActor("Model/�������J.pmd", *_pmdRenderer, *_dx12));
	_pmdActors[2].reset(new PMDActor("Model/�㉹�n�N.pmd", *_pmdRenderer, *_dx12));

	return true;
}

//���s
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

		//�A�v���P�[�V�����I����
		if (msg.message == WM_QUIT)
			break;

		//�S�̂̕`�揀��
		_dx12->BeginDraw();

		//PMD�p�̕`��p�C�v���C���ɍ��킹��
		_dx12->CommandList()->SetPipelineState(_pmdRenderer->GetPipelineState());

		//���[�g�V�O�l�`����PMD�p�ɍ��킹��
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

		//�t���b�v
		_dx12->Swapchain()->Present(1, 0);
	}
}

//�I��
void RefacApplication::Terminate()
{
	//�N���X�̓o�^����
	UnregisterClass(_windowClass.lpszClassName, _windowClass.hInstance);
}


//�V���O���g���C���X�^���X�擾
RefacApplication& RefacApplication::Instance()
{
	static RefacApplication instance;
	return instance;
}
