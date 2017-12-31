//http://www.freebuf.com/articles/system/93413.html
//http://www.freebuf.com/articles/system/94693.html
#include<vector>
#include<Windows.h>
#include<TlHelp32.h>

DWORD GetFirstThreadID(DWORD pid)
{
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h == INVALID_HANDLE_VALUE)
		return 0;
	THREADENTRY32 te;
	te.dwSize = sizeof(te);
	for (BOOL tfound = Thread32First(h, &te); tfound; tfound = Thread32Next(h, &te))
	{
		if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
		{
			if (te.th32OwnerProcessID == pid)
			{
				HANDLE hThread = OpenThread(READ_CONTROL, FALSE, te.th32ThreadID);
				if (hThread)
					return te.th32ThreadID;
			}
		}
	}
	CloseHandle(h);
	return 0;
}

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
	ShellExecuteEx(&se);
	DWORD hTargetPid = GetProcessId(se.hProcess);
	//貌似不能100%成功？
	HHOOK hhkMidi = SetWindowsHookEx(WH_DEBUG, fMidiHook, hdll, GetFirstThreadID(hTargetPid));
	if (hhkMidi == NULL)
	{
		TCHAR msg[40];
		wsprintf(msg, TEXT("无法设置Hook：%#x"), GetLastError());
		MessageBox(NULL, msg, NULL, MB_ICONERROR);
	}
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