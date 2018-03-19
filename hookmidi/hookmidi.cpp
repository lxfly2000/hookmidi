//http://www.freebuf.com/articles/system/93413.html
//http://www.freebuf.com/articles/system/94693.html
#include<vector>
#include<Windows.h>
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

bool ChooseExecuteFile(std::wstring &path)
{
	TCHAR cpath[MAX_PATH] = TEXT(""), cname[MAX_PATH] = TEXT("");
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof OPENFILENAME;
	ofn.hwndOwner = nullptr;
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = TEXT("应用程序\0*.exe;*.com\0所有文件\0*\0\0");
	ofn.lpstrFile = cpath;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = cname;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY;
	if (GetOpenFileName(&ofn))
	{
		path = cpath;
		return true;
	}
	return false;
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE hPvI, LPWSTR param, int nShow)
{
	SHELLEXECUTEINFO se = { 0 };
	se.cbSize = sizeof se;
	se.hwnd = NULL;
	se.lpVerb = TEXT("open");
	se.lpFile = NULL;
	se.lpParameters = NULL;
	se.fMask = SEE_MASK_NOCLOSEPROCESS;
	se.nShow = SW_SHOW;
	TCHAR szcd[MAX_PATH];
	GetCurrentDirectory(ARRAYSIZE(szcd), szcd);
	se.lpDirectory = szcd;

	std::wstring separam, sefullpath;
	switch (__argc)
	{
	default:
	case 3:
		for (int i = 2; i < __argc; i++)
		{
			if (i > 2)
				separam += TEXT(" ");
			separam += __wargv[i];
		}
		se.lpParameters = separam.c_str();
	case 2:
	{
		TCHAR szfp[MAX_PATH];
		GetFullPathName(__wargv[1], ARRAYSIZE(szfp), szfp, NULL);
		sefullpath = szfp;
		se.lpFile = sefullpath.c_str();
	}
	break;
	case 1:
		if (!ChooseExecuteFile(separam))
			return 0;
		GetCurrentDirectory(ARRAYSIZE(szcd), szcd);
		se.lpFile = separam.c_str();
		break;
	case 0:
		return E_INVALIDARG;
		break;
	}
	HMODULE hdll = LoadLibrary(TEXT("hijackmidi"));
	HOOKPROC fMidiHook = (HOOKPROC)GetProcAddress(hdll, "MidiHook");
	if (fMidiHook == NULL)
		MessageBox(NULL, TEXT("没有找到 MidiHook 函数。"), NULL, MB_ICONERROR);
	//个人推测WH_DEBUG开销最大而WH_SHELL开销最小
	//实际上这样改过后就不需要监视目标进程了，因为钩子是全局的
	//顺序上只要保证钩子在目标程序前启动就行了
	HHOOK hhkMidi = SetWindowsHookEx(WH_SHELL, fMidiHook, hdll, 0);
	if (hhkMidi == NULL)
	{
		TCHAR msg[40];
		wsprintf(msg, TEXT("无法设置Hook：%#x\n请尝试重新启动。"), GetLastError());
		MessageBox(NULL, msg, NULL, MB_ICONERROR);
	}
	ShellExecuteEx(&se);
	WaitForSingleObject(se.hProcess, INFINITE);
	if (!UnhookWindowsHookEx(hhkMidi))
	{
		TCHAR msg[40];
		DWORD e = GetLastError();
		if (e)
		{
			wsprintf(msg, TEXT("移除Hook时出错：%#x"), e);
			MessageBox(NULL, msg, NULL, MB_ICONERROR);
			return e;
		}
	}

	return 0;
}