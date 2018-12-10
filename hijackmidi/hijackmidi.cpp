#include"ChooseList.h"
#include<vector>
#include<Windows.h>
#include"minhook\include\MinHook.h"
#pragma comment(lib,"winmm.lib")

#define APP_NAME "midihook"
#define INI_FILE ".\\midihook.ini"
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
	MIDIOUTCAPS moc;
	UINT midiOutCount = midiOutGetNumDevs();
	midiOutGetDevCaps(defaultID, &moc, sizeof moc);
	std::vector<std::wstring> descptr(midiOutCount + 1);
	for (int i = -1; i < (int)midiOutCount; i++)
	{
		midiOutGetDevCaps(i, &moc, sizeof moc);
		TCHAR devdesc[128]=TEXT(""),appendText[32]=TEXT("");
		wsprintf(devdesc, TEXT("[%d]%s"), i, moc.szPname);
		if ((UINT)i == MIDI_MAPPER)
			lstrcat(appendText, TEXT("MIDI 映射器"));
		if ((UINT)i == defaultID)
		{
			if (defaultID == MIDI_MAPPER)
				lstrcat(appendText, TEXT("，"));
			lstrcat(appendText, TEXT("程序指定的设备"));
		}
		if (lstrlen(appendText) > 0)
		{
			lstrcat(devdesc, TEXT("（"));
			lstrcat(devdesc, appendText);
			lstrcat(devdesc, TEXT("）"));
		}
		descptr[i + 1] = devdesc;
	}

	BOOL bRem = FALSE;
	std::vector<LPCTSTR> ptrs;
	for (std::wstring&str : descptr)
		ptrs.push_back(str.c_str());
	defaultID = ChooseList(NULL, TEXT("选择MIDI输出设备"), ptrs.data(), midiOutCount + 1, defaultID + 1, TEXT("下次不再提示(&R)"), &bRem) - 1;
	if (bRem)
	{
		SetRemember(TRUE);
		SetRememberDeviceID(defaultID);
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
