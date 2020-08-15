//#define HOOKMIDI
#define MAX_FUNCNAME 256
#define APP_IDENTIFIER "Hook"
#define KEY_HOOK_FILE "file"
#define KEY_HOOK_FUNC "function"
#define KEY_HOOK_TYPE "type"
#include<Windows.h>
#ifdef HOOKMIDI
#define DEFAULT_HOOK_FILE "hijackmidi.dll"
#define DEFAULT_HOOK_FUNC "MidiHook"
#define DEFAULT_HOOK_TYPE WH_SHELL
#else
#define DEFAULT_HOOK_FILE "hook.dll"
#define DEFAULT_HOOK_FUNC "HookCallback"
#define DEFAULT_HOOK_TYPE WH_MSGFILTER
#endif
//http://www.freebuf.com/articles/system/93413.html
//http://www.freebuf.com/articles/system/94693.html
#include<windowsx.h>
#include"resource.h"
#include"GetExportTable.h"
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

WCHAR _profileNameW[MAX_PATH];
char _profileNameA[MAX_PATH];
LPCWSTR GetProfileNameW()
{
	if (!GetModuleFileNameW(GetModuleHandleW(NULL), _profileNameW, ARRAYSIZE(_profileNameW) - 1))
		return NULL;
	LPWSTR pdot = wcsrchr(_profileNameW, '.');
	if (pdot)
		wcscpy_s(pdot, 5, L".ini");
	return _profileNameW;
}
LPCSTR GetProfileNameA()
{
	if (!GetModuleFileNameA(GetModuleHandleA(NULL), _profileNameA, ARRAYSIZE(_profileNameA) - 1))
		return NULL;
	LPSTR pdot = strrchr(_profileNameA, '.');
	if (pdot)
		strcpy_s(pdot, 5, ".ini");
	return _profileNameA;
}
#ifdef _UNICODE
#define GetProfileName GetProfileNameW
#else
#define GetProfileName GetProfileNameA
#endif

typedef struct tagHookTypeItem
{
	int typeId;
	LPCTSTR descriptionText;
}HookTypeItem;

const HookTypeItem hookTypes[] = {
	{WH_MSGFILTER,TEXT("[-1]��Ϣ������")},
	{WH_JOURNALRECORD,TEXT("[0]���¼")},
	{WH_JOURNALPLAYBACK,TEXT("[1]��ط�")},
	{WH_KEYBOARD,TEXT("[2]����")},
	{WH_GETMESSAGE,TEXT("[3]��Ϣ����")},
	{WH_CALLWNDPROC,TEXT("[4]���ڹ���")},
	{WH_CBT,TEXT("[5]ϵͳ������Ϣ")},
	{WH_SYSMSGFILTER,TEXT("[6]ϵͳ��Ϣ������")},
	{WH_MOUSE,TEXT("[7]���")},
	{WH_DEBUG,TEXT("[9]����")},
	{WH_SHELL,TEXT("[10]���Ӧ�ó���")},
	{WH_FOREGROUNDIDLE,TEXT("[11]����ǰ̨�߳̿���")},
	{WH_CALLWNDPROCRET,TEXT("[12]���ڹ��̷���")},
	{WH_KEYBOARD_LL,TEXT("[13]���̵ײ�")},
	{WH_MOUSE_LL,TEXT("[14]���ײ�")}
};

int GetIndexOfHookType(int type)
{
	int index = 0;
	for (int i = 0; i < ARRAYSIZE(hookTypes); i++)
		if (hookTypes[i].typeId == type)
			index = i;
	return index;
}

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
		TCHAR msg[21];
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
	if (lstrcmpi(wcsrchr(dllfile, '.'), TEXT(".dll")) != 0)
	{
		if (MessageBox(hDlg, TEXT("��ѡ����ļ������� DLL �ļ���ȷ��������"), GetFileName(dllfile), MB_ICONWARNING | MB_OKCANCEL) != IDOK)
			return;
	}
	GetDlgItemTextA(hDlg, IDC_COMBO_HOOKFUNC, funcname, ARRAYSIZE(funcname));
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
	int chosenType = hookTypes[ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMBO_HOOKTYPE))].typeId;
	hHook = SetWindowsHookEx(chosenType, fHookCallback, hDll, 0);
	if (hHook == NULL)
	{
		StopHook();
		TCHAR msg[53];
		wsprintf(msg, TEXT("�޷�����Hook��%#x\n�볢�����������ó���������ǳ���������Ǹ����͵Ĺ��Ӳ���֧�֡�"), GetLastError());
		MessageBox(hDlg, msg, NULL, MB_ICONERROR);
		return;
	}
	TCHAR title[533];
	wsprintf(title, TEXT("Hook ��������%s, %S [%d]"), GetFileName(dllfile), funcname, chosenType);
	SetWindowText(hDlg, title);
	SetDlgItemText(hDlg, IDOK, TEXT("ֹͣ"));
#ifndef HOOKMIDI
	WritePrivateProfileStringA(APP_IDENTIFIER, KEY_HOOK_FUNC, funcname, GetProfileNameA());
	WritePrivateProfileString(TEXT(APP_IDENTIFIER), TEXT(KEY_HOOK_FILE), dllfile, GetProfileName());
	TCHAR *const typeStr = title;
	wsprintf(typeStr, TEXT("%d"), chosenType);
	WritePrivateProfileString(TEXT(APP_IDENTIFIER), TEXT(KEY_HOOK_TYPE), typeStr, GetProfileName());
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
	return SetDlgItemText(hDlg, IDC_COMBO_HOOKFUNC, funcname);
}

int SetHookType(int type)
{
	return ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_HOOKTYPE), GetIndexOfHookType(type));
}

void DialogInit(HWND h)
{
	hDlg = h;
	Edit_LimitText(GetDlgItem(h, IDC_EDIT_HOOKFILE), MAX_PATH - 1);
	ComboBox_LimitText(GetDlgItem(h, IDC_COMBO_HOOKFUNC), MAX_FUNCNAME - 1);
	for (auto &e : hookTypes)
		ComboBox_AddString(GetDlgItem(h, IDC_COMBO_HOOKTYPE), e.descriptionText);

	TCHAR buf[MAX_PATH];
	GetPrivateProfileString(TEXT(APP_IDENTIFIER), TEXT(KEY_HOOK_FILE), TEXT(DEFAULT_HOOK_FILE),
		buf, ARRAYSIZE(buf), GetProfileName());
	SetHookFile(buf);
	GetPrivateProfileString(TEXT(APP_IDENTIFIER), TEXT(KEY_HOOK_FUNC), TEXT(DEFAULT_HOOK_FUNC),
		buf, ARRAYSIZE(buf), GetProfileName());
	SetHookFunction(buf);
	SetHookType(GetPrivateProfileInt(TEXT(APP_IDENTIFIER), TEXT(KEY_HOOK_TYPE), DEFAULT_HOOK_TYPE, GetProfileName()));

	StopHook();
	switch (__argc)
	{
	case 4:
		SetHookType(_wtoi(__wargv[3]));
	case 3:
		SetHookFunction(__wargv[2]);
	case 2:
		SetHookFile(__wargv[1]);
		break;
	}
	if (__argc > 2)
		StartHook();
}

INT_PTR WINAPI DialogCallback(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m)
	{
	case WM_INITDIALOG:
		DialogInit(h);
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
		case IDC_EDIT_HOOKFILE:
			if (HIWORD(w) == EN_CHANGE)
			{
				HWND hctl = GetDlgItem(h, IDC_COMBO_HOOKFUNC);
				int curSel = ComboBox_GetCurSel(hctl);
				while (ComboBox_GetCount(hctl))
					ComboBox_DeleteString(hctl, 0);
				char path[MAX_PATH]{};
				GetDlgItemTextA(h, IDC_EDIT_HOOKFILE, path, ARRAYSIZE(path) - 1);
				auto funclist = GetExportTable(path);
				for (size_t i = 0; i < funclist.size(); i++)
				{
#ifdef _UNICODE
					TCHAR fn[256]{};
					MultiByteToWideChar(CP_ACP, 0, funclist[i].c_str(), (int)funclist[i].size(), fn, ARRAYSIZE(fn) - 1);
					ComboBox_AddString(hctl, fn);
#else
					ComboBox_AddString(hctl, funclist[i].c_str());
#endif
					if (i == 0 && (curSel != CB_ERR || ComboBox_GetTextLength(hctl) == 0))
						ComboBox_SetCurSel(hctl, 0);
				}
			}
			break;
		}
		break;
	case WM_DROPFILES:
	{
		TCHAR path[MAX_PATH];
		DragQueryFile((HDROP)w, 0, path, MAX_PATH);
		DragFinish((HDROP)w);
		SetHookFile(path);
	}
		break;
	}
	return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hI, _In_opt_ HINSTANCE hPvI, _In_ LPWSTR param, _In_ int nShow)
{
	return (int)DialogBox(hI, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, DialogCallback);
}