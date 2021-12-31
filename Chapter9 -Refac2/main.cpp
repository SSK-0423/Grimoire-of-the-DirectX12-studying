#include "Application.h"
#include "RefacApplication.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	auto& app = RefacApplication::Instance();
	if (!app.Init())
	{
		return -1;
	}

	app.Run();
	app.Terminate();

	return 0;
}