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
	ofn.lpstrFilter = TEXT("Ӧ�ó���\0*.exe;*.com\0�����ļ�\0*\0\0");
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
		MessageBox(NULL, TEXT("û���ҵ� MidiHook ������"), NULL, MB_ICONERROR);
	//�����Ʋ�WH_DEBUG��������WH_SHELL������С
	//ʵ���������Ĺ���Ͳ���Ҫ����Ŀ������ˣ���Ϊ������ȫ�ֵ�
	//˳����ֻҪ��֤������Ŀ�����ǰ����������
	HHOOK hhkMidi = SetWindowsHookEx(WH_SHELL, fMidiHook, hdll, 0);
	if (hhkMidi == NULL)
	{
		TCHAR msg[40];
		wsprintf(msg, TEXT("�޷�����Hook��%#x\n�볢������������"), GetLastError());
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
			wsprintf(msg, TEXT("�Ƴ�Hookʱ����%#x"), e);
			MessageBox(NULL, msg, NULL, MB_ICONERROR);
			return e;
		}
	}

	return 0;
}