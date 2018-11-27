#include<vector>
#include<Windows.h>
#include<CommCtrl.h>
#include"minhook\include\MinHook.h"
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"ComCtl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define APP_NAME "hijackmidi"
#define INI_FILE ".\\hijackmidi.ini"
#define KEY_REMEMBER "remember"
#define KEY_DEVICE_ID "deviceId"

BOOL GetRemember()
{
	return GetPrivateProfileInt(TEXT(APP_NAME), TEXT(KEY_REMEMBER), FALSE, TEXT(INI_FILE));
}

BOOL SetRemember(BOOL b)
{
	TCHAR szb[2];
	wsprintf(szb, TEXT("%d"), b);
	return WritePrivateProfileString(TEXT(APP_NAME), TEXT(KEY_REMEMBER), szb, TEXT(INI_FILE));
}

UINT GetRememberDeviceID()
{
	return GetPrivateProfileInt(TEXT(APP_NAME), TEXT(KEY_DEVICE_ID), MIDI_MAPPER, TEXT(INI_FILE));
}

BOOL SetRememberDeviceID(UINT deviceId)
{
	TCHAR szid[12];
	wsprintf(szid, TEXT("%d"), deviceId);
	return WritePrivateProfileString(TEXT(APP_NAME), TEXT(KEY_DEVICE_ID), szid, TEXT(INI_FILE));
}

UINT ChooseMidiOutDevice(UINT defaultID)
{
	if (GetRemember())
		return GetRememberDeviceID();
	UINT selID = defaultID;
	MIDIOUTCAPS moc;
	TCHAR msg[64];
	TASKDIALOGCONFIG tc = { 0 };
	std::vector<TASKDIALOG_BUTTON> tb;
	tc.cbSize = sizeof tc;
	tc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
	tc.hInstance = NULL;
	tc.pszContent = msg;
	tc.pszMainInstruction = TEXT("选择MIDI输出设备");
	tc.pszMainIcon = TD_INFORMATION_ICON;
	tc.dwFlags = TDF_USE_COMMAND_LINKS;
	tc.pszVerificationText = TEXT("下次不再提示(&R)");

	UINT midiOutCount = midiOutGetNumDevs();
	midiOutGetDevCaps(defaultID, &moc, sizeof moc);
	wsprintf(msg, TEXT("程序指定的设备为：\n[%d]%s"), defaultID, moc.szPname);
	std::vector<std::wstring> descptr(midiOutCount + 1);
	for (int i = -1; i < (int)midiOutCount; i++)
	{
		midiOutGetDevCaps(i, &moc, sizeof moc);
		TCHAR devdesc[60];
		wsprintf(devdesc, TEXT("[&%d]%s"), i, moc.szPname);
		if ((UINT)i == MIDI_MAPPER)
			lstrcat(devdesc, TEXT("\nMIDI 映射器"));
		if ((UINT)i == defaultID)
		{
			if (defaultID == MIDI_MAPPER)
				lstrcat(devdesc, TEXT("，"));
			else
				lstrcat(devdesc, TEXT("\n"));
			lstrcat(devdesc, TEXT("程序指定的设备"));
		}
		descptr[i + 1] = devdesc;
		tb.push_back({ 1000 + i,descptr[i + 1].c_str() });
	}
	tc.pButtons = tb.data();
	tc.cButtons = (UINT)tb.size();

	BOOL bRem = FALSE;
	TaskDialogIndirect(&tc, reinterpret_cast<int*>(&selID), NULL, &bRem);
	if (selID >= 999)
	{
		selID -= 1000;
		if (bRem)
		{
			SetRemember(TRUE);
			SetRememberDeviceID(selID);
		}
		return selID;
	}
	return defaultID;
}

BOOL StartHijack();
BOOL StopHijack();

MMRESULT WINAPI Redirect_midiOutOpen(LPHMIDIOUT phmo, UINT uDeviceID, DWORD_PTR dwCallback,
	DWORD_PTR dwInstance, DWORD fdwOpen)
{
	uDeviceID = ChooseMidiOutDevice(uDeviceID);
	StopHijack();
	uDeviceID = midiOutOpen(phmo, uDeviceID, dwCallback, dwInstance, fdwOpen);
	StartHijack();
	return uDeviceID;
}

decltype(midiOutOpen)* funcOriginal = nullptr;

BOOL StartHijack()
{
	if (MH_EnableHook(&midiOutOpen) != MH_OK)
		return FALSE;
	return TRUE;
}

BOOL StopHijack()
{
	if (MH_DisableHook(&midiOutOpen) != MH_OK)
		return FALSE;
	return TRUE;
}

BOOL InitHijack()
{
	if (MH_Initialize() != MH_OK)
		return FALSE;
	if (MH_CreateHook(&midiOutOpen, &Redirect_midiOutOpen, reinterpret_cast<void**>(&funcOriginal)) != MH_OK)
		return FALSE;
	return TRUE;
}

BOOL UninitHijack()
{
	if (MH_Uninitialize() != MH_OK)
		return FALSE;
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hInstDll);
		InitHijack();
		StartHijack();
		break;
	case DLL_PROCESS_DETACH:
		StopHijack();
		UninitHijack();
		break;
	case DLL_THREAD_ATTACH:break;
	case DLL_THREAD_DETACH:break;
	}
	return TRUE;
}

#ifdef _WIN64
#pragma comment(linker,"/Export:MidiHook")
#else
#pragma comment(linker,"/Export:MidiHook=_MidiHook@12")
#endif
extern "C" LRESULT WINAPI MidiHook(int code, WPARAM w, LPARAM l)
{
	return CallNextHookEx(NULL, code, w, l);
}
