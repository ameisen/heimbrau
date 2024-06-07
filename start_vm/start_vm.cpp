#include <Windows.h>

int main ()
{
	HWND hWnd = GetConsoleWindow();
	ShowWindow( hWnd, SW_HIDE );

	PROCESS_INFORMATION info;
	STARTUPINFO sinfo;
	GetStartupInfo(&sinfo);
	CreateProcess(
		"C:\\Program Files\\Oracle\\VirtualBox\\VBoxManage.exe",
		"VBoxManage.exe startvm HyperK",
		nullptr, nullptr, false, 0, nullptr, "C:\\Program Files\\Oracle\\VirtualBox", &sinfo, &info
	);

	return 0;
}