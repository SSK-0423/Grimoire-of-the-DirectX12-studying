#pragma once
#include <Windows.h>
#include <tchar.h>
#include <memory>
#include <vector>

class Dx12Wrapper;
class PMDActor;
class PMDRenderer;

class RefacApplication
{
private:
	HWND _hwnd;
	WNDCLASSEX _windowClass;
	std::shared_ptr<Dx12Wrapper> _dx12;
	std::shared_ptr<PMDActor> _pmdActor;
	std::vector<std::shared_ptr<PMDActor>> _pmdActors;
	std::shared_ptr<PMDRenderer> _pmdRenderer;

	RefacApplication() {}
public:
	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass);
	SIZE GetWindowSize() const;
	bool Init();
	void Run();
	void Terminate();
	static RefacApplication& Instance();
};