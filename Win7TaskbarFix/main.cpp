#include <Windows.h>
#include <stdexcept>
#include <sstream>
#include <tchar.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

NOTIFYICONDATA g_TrayIcon;
HHOOK g_hMouse;


BOOL IsFullScreenAppRunning()
{
	HWND hWnd = GetForegroundWindow();

	RECT rc;
	GetWindowRect(hWnd, &rc);

	if (rc.right - rc.left == GetSystemMetrics(SM_CXSCREEN) && rc.bottom - rc.top == GetSystemMetrics(SM_CYSCREEN))
		return TRUE;

	return FALSE;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT* pms = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

	if (wParam == WM_MOUSEMOVE)
	{
		if (pms->pt.x < 2) // check if mouse x position is close to the left edge
		{
			if ( !IsFullScreenAppRunning() )
			{
				HWND taskbar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
				SwitchToThisWindow(taskbar, TRUE);
			}
		}
	}

	return CallNextHookEx(g_hMouse, nCode, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try {

	WNDCLASSEX wx = {};
	LPCTSTR lpszClassName = TEXT("Win7TaskbarFix");
	wx.cbSize = sizeof(wx);
	wx.lpfnWndProc = WndProc;
	wx.hInstance = hInstance;
	wx.lpszClassName = lpszClassName;

	int iResult = RegisterClassEx(&wx);
	if (iResult == 0)
		throw std::runtime_error("RegisterClassEx returned 0");

	HWND hWnd = CreateWindowEx(0, lpszClassName, TEXT(""), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (hWnd == 0)
		throw std::runtime_error("CreateWindowEx returned 0");
	
	g_TrayIcon.cbSize = sizeof(g_TrayIcon);
	g_TrayIcon.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	g_TrayIcon.hWnd = hWnd;
	g_TrayIcon.uCallbackMessage = WM_USER + 200;
	g_TrayIcon.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE;
	g_TrayIcon.uID = 1;
	_tcscpy_s(g_TrayIcon.szTip, TEXT("Taskbar autohide fix"));

	iResult = Shell_NotifyIcon(NIM_ADD, &g_TrayIcon);
	if(iResult == FALSE)
		throw std::runtime_error("Shell_NotifyIcon returned FALSE");

	// setting up mouse hook
	g_hMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// exception handler
	} catch (const std::exception& e) {
		std::ostringstream ssErrorMsg;
		ssErrorMsg << e.what() << "\n"
			"GLE: " << GetLastError();
		MessageBoxA(NULL, ssErrorMsg.str().c_str(), "Error in WinMain", MB_ICONERROR);
		return -1;
	}

	// cleanup
	Shell_NotifyIcon(NIM_DELETE, &g_TrayIcon);
	UnhookWindowsHookEx(g_hMouse);

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_USER + 200:
	{
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			PostQuitMessage(0); // exit WinMain message loop on double click allowing for proper cleanup
			break;

		default:
			break;
		}
	}
	break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
