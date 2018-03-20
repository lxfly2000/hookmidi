//#define HOOKMIDI
#define MAX_FUNCNAME 256
#define APP_IDENTIFIER "Hook"
#define PROFILE_NAME ".\\hook.ini"
#define KEY_HOOK_FILE "file"
#define KEY_HOOK_FUNC "function"
#ifdef HOOKMIDI
#define DEFAULT_HOOK_FILE "hijackmidi.dll"
#define DEFAULT_HOOK_FUNC "MidiHook"
#else
#define DEFAULT_HOOK_FILE "hook.dll"
#define DEFAULT_HOOK_FUNC "HookCallback"
#endif
//http://www.freebuf.com/articles/system/93413.html
//http://www.freebuf.com/articles/system/94693.html
#include<Windows.h>
#include<windowsx.h>
#include"resource.h"
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

BOOL ChooseHookFile(HWND hParent, LPTSTR path, int lpath)
{
	TCHAR cname[MAX_PATH] = TEXT("");
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof OPENFILENAME;
	ofn.hwndOwner = hParent;
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = TEXT("��̬���ӿ�\0*.dll\0\0");
	ofn.lpstrFile = path;
	ofn.nMaxFile = lpath;
	ofn.lpstrFileTitle = cname;
	ofn.nMaxFileTitle = ARRAYSIZE(cname);
	ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	return GetOpenFileName(&ofn);
}

LPCTSTR GetFileName(LPCTSTR path)
{
	LPCTSTR p = wcsrchr(path, '\\');
	if (!p)
		p = wcsrchr(path, '/');
	if (!p)
		return path;
	return p + 1;
}

HHOOK hHook = NULL;
HMODULE hDll = NULL;
HWND hDlg = NULL;

void StopHook()
{
	if (hHook == NULL)
		return;
	if (!UnhookWindowsHookEx(hHook))
	{
		TCHAR msg[40];
		DWORD e = GetLastError();
		if (e)
		{
			wsprintf(msg, TEXT("�Ƴ�Hookʱ����%#x"), e);
			MessageBox(NULL, msg, NULL, MB_ICONERROR);
			return;
		}
	}
	hHook = NULL;
	if (hDll)
	{
		if (!FreeLibrary(hDll))
		{
			MessageBox(hDlg, TEXT("�ͷ� DLL ʱ����"), NULL, MB_ICONERROR);
			return;
		}
		hDll = NULL;
	}
	SetWindowText(hDlg, TEXT("Hook ��ֹͣ"));
	SetDlgItemText(hDlg, IDOK, TEXT("����"));
}

void StartHook()
{
	if (hHook)
	{
		MessageBox(hDlg, TEXT("Hook �Ѿ�������"), NULL, MB_ICONINFORMATION);
		return;
	}
	TCHAR dllfile[MAX_PATH];
	char funcname[MAX_FUNCNAME];
	GetDlgItemText(hDlg, IDC_EDIT_HOOKFILE, dllfile, ARRAYSIZE(dllfile));
	GetDlgItemTextA(hDlg, IDC_EDIT_HOOKFUNC, funcname, ARRAYSIZE(funcname));
	if (!hDll)
	{
		hDll = LoadLibrary(dllfile);
		if (!hDll)
		{
			StopHook();
			MessageBox(hDlg, TEXT("�޷����� DLL �ļ���"), NULL, MB_ICONERROR);
			return;
		}
	}
	HOOKPROC fHookCallback = (HOOKPROC)GetProcAddress(hDll, funcname);
	if (!fHookCallback)
	{
		StopHook();
		MessageBox(hDlg, TEXT("�Ҳ���ָ���ĺ�������"), NULL, MB_ICONERROR);
		return;
	}
	//�����Ʋ�WH_DEBUG��������WH_SHELL������С
	//ʵ���������Ĺ���Ͳ���Ҫ����Ŀ������ˣ���Ϊ������ȫ�ֵ�
	//˳����ֻҪ��֤������Ŀ�����ǰ����������
	hHook = SetWindowsHookEx(WH_SHELL, fHookCallback, hDll, 0);
	if (hHook == NULL)
	{
		StopHook();
		TCHAR msg[40];
		wsprintf(msg, TEXT("�޷�����Hook��%#x\n�볢������������"), GetLastError());
		MessageBox(hDlg, msg, NULL, MB_ICONERROR);
		return;
	}
	TCHAR title[300];
	wsprintf(title, TEXT("Hook ��������%s"), GetFileName(dllfile));
	SetWindowText(hDlg, title);
	SetDlgItemText(hDlg, IDOK, TEXT("ֹͣ"));
#ifndef HOOKMIDI
	WritePrivateProfileStringA(APP_IDENTIFIER, KEY_HOOK_FUNC, funcname, PROFILE_NAME);
	WritePrivateProfileString(TEXT(APP_IDENTIFIER), TEXT(KEY_HOOK_FILE), dllfile, TEXT(PROFILE_NAME));
#endif
}

void ToggleHook()
{
	if (hHook == NULL)
		StartHook();
	else
		StopHook();
}

BOOL SetHookFile(LPCTSTR path)
{
	return SetDlgItemText(hDlg, IDC_EDIT_HOOKFILE, path);
}

BOOL SetHookFunction(LPCTSTR funcname)
{
	return SetDlgItemText(hDlg, IDC_EDIT_HOOKFUNC, funcname);
}

INT_PTR WINAPI DialogCallback(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m)
	{
	case WM_INITDIALOG:
		hDlg = h;
		Edit_LimitText(GetDlgItem(h, IDC_EDIT_HOOKFILE), MAX_PATH-1);
		Edit_LimitText(GetDlgItem(h, IDC_EDIT_HOOKFUNC), MAX_FUNCNAME-1);
		{
			TCHAR buf[MAX_PATH];
			GetPrivateProfileString(TEXT(APP_IDENTIFIER), TEXT(KEY_HOOK_FILE), TEXT(DEFAULT_HOOK_FILE),
				buf, ARRAYSIZE(buf), TEXT(PROFILE_NAME));
			SetHookFile(buf);
			GetPrivateProfileString(TEXT(APP_IDENTIFIER), TEXT(KEY_HOOK_FUNC), TEXT(DEFAULT_HOOK_FUNC),
				buf, ARRAYSIZE(buf), TEXT(PROFILE_NAME));
			SetHookFunction(buf);
		}
		StopHook();
		switch (__argc)
		{
		case 3:
			SetHookFunction(__wargv[2]);
		case 2:
			SetHookFile(__wargv[1]);
			break;
		}
		if (__argc > 2)
			StartHook();
		break;
	case WM_COMMAND:
		switch (LOWORD(w))
		{
		case IDOK:
			ToggleHook();
			break;
		case IDCANCEL:
			StopHook();
			EndDialog(h, 0);
			break;
		case IDC_BUTTON_BROWSE:
		{
			TCHAR path[MAX_PATH];
			GetDlgItemText(h, IDC_EDIT_HOOKFILE, path, MAX_PATH);
			if (ChooseHookFile(h, path, MAX_PATH))
				SetHookFile(path);
		}
			break;
		}
		break;
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE hPvI, LPWSTR param, int nShow)
{
	return (int)DialogBox(hI, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, DialogCallback);
}