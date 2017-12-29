#ifndef WIN32
#error �ó���Ŀǰֻ����x86�б��롣
#endif

#include<vector>
#include<Windows.h>
#include<CommCtrl.h>
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"ComCtl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

UINT ChooseMidiOutDevice(UINT defaultID)
{
	UINT selID = defaultID;
	MIDIOUTCAPS moc;
	TCHAR msg[64];
	TASKDIALOGCONFIG tc = { 0 };
	std::vector<TASKDIALOG_BUTTON> tb;
	tc.cbSize = sizeof tc;
	tc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
	tc.hInstance = NULL;
	tc.pszContent = msg;
	tc.pszMainInstruction = TEXT("ѡ��MIDI����豸");
	tc.pszMainIcon = TD_INFORMATION_ICON;
	tc.dwFlags = TDF_USE_COMMAND_LINKS;

	UINT midiOutCount = midiOutGetNumDevs();
	midiOutGetDevCaps(defaultID, &moc, sizeof moc);
	wsprintf(msg, TEXT("����ָ�����豸Ϊ��\n[%d]%s"), defaultID, moc.szPname);
	std::vector<std::wstring> descptr(midiOutCount + 1);
	for (int i = -1; i < (int)midiOutCount; i++)
	{
		midiOutGetDevCaps(i, &moc, sizeof moc);
		TCHAR devdesc[60];
		wsprintf(devdesc, TEXT("[%d]%s"), i, moc.szPname);
		if ((UINT)i == MIDI_MAPPER)
			lstrcat(devdesc, TEXT("\nMIDI ӳ����"));
		if ((UINT)i == defaultID)
		{
			if (defaultID == MIDI_MAPPER)
				lstrcat(devdesc, TEXT("��"));
			else
				lstrcat(devdesc, TEXT("\n"));
			lstrcat(devdesc, TEXT("����ָ�����豸"));
		}
		descptr[i + 1] = devdesc;
		tb.push_back({ 1000 + i,descptr[i + 1].c_str() });
	}
	tc.pButtons = tb.data();
	tc.cButtons = tb.size();

	TaskDialogIndirect(&tc, reinterpret_cast<int*>(&selID), NULL, NULL);
	if (selID >= 999)
		return selID - 1000;
	return defaultID;
}

void StartHijack();
void StopHijack();

MMRESULT WINAPI Redirect_midiOutOpen(LPHMIDIOUT phmo, UINT uDeviceID, DWORD_PTR dwCallback,
	DWORD_PTR dwInstance, DWORD fdwOpen)
{
	uDeviceID = ChooseMidiOutDevice(uDeviceID);
	StopHijack();
	uDeviceID = midiOutOpen(phmo, uDeviceID, dwCallback, dwInstance, fdwOpen);
	StartHijack();
	return uDeviceID;
}

using FUNCTYPE = decltype(midiOutOpen);
FUNCTYPE *funcOriginal = nullptr;
DWORD protectOriginal;
char apientry[5];
DWORD hTargetPid;
HANDLE hTargetProcess;

void StartHijack()
{
	hTargetProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, hTargetPid);
	funcOriginal = (FUNCTYPE*)GetProcAddress(LoadLibrary(TEXT("winmm.dll")), "midiOutOpen");
	memcpy(apientry, funcOriginal, 5);
	DWORD entryaddr = reinterpret_cast<DWORD>(Redirect_midiOutOpen) - reinterpret_cast<DWORD>(funcOriginal) - 5;
	char asmcode[5] = { '\xe9' };//JMP
	memcpy(asmcode + 1, &entryaddr, 4);
	VirtualProtectEx(hTargetProcess, funcOriginal, 5, PAGE_EXECUTE_READWRITE, &protectOriginal);
	WriteProcessMemory(hTargetProcess, funcOriginal, asmcode, 5, NULL);
	VirtualProtectEx(hTargetProcess, funcOriginal, 5, protectOriginal, &protectOriginal);
}

void StopHijack()
{
	VirtualProtectEx(hTargetProcess, funcOriginal, 5, PAGE_EXECUTE_READWRITE, &protectOriginal);
	WriteProcessMemory(hTargetProcess, funcOriginal, apientry, 5, NULL);
	VirtualProtectEx(hTargetProcess, funcOriginal, 5, protectOriginal, &protectOriginal);
	CloseHandle(hTargetProcess);
}

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hInstDll);
		hTargetPid = GetCurrentProcessId();
		StartHijack();
		break;
	case DLL_PROCESS_DETACH:
		StopHijack();
		break;
	case DLL_THREAD_ATTACH:break;
	case DLL_THREAD_DETACH:break;
	}
	return TRUE;
}

#pragma comment(linker,"/Export:MidiHook=_MidiHook@12")
extern "C" LRESULT WINAPI MidiHook(int code, WPARAM w, LPARAM l)
{
	return CallNextHookEx(NULL, code, w, l);
}
