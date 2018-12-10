#include "ChooseList.h"
#include "ChooseListRes.h"
#include <WindowsX.h>

LPCTSTR *listOptions, dlgTitle,dlgCheckText;
int countOptions, chosenOption;
BOOL chosenCheck;

INT_PTR CALLBACK ChooseCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		if (dlgTitle)
			SetWindowText(hwnd, dlgTitle);
		if (dlgCheckText)
			SetDlgItemText(hwnd, IDC_CHECK_OPTION, dlgCheckText);
		CheckDlgButton(hwnd, IDC_CHECK_OPTION, chosenCheck);
		for (int i = 0; i < countOptions; i++)
		{
			HWND hList = GetDlgItem(hwnd, IDC_LIST_CHOOSE);
			ListBox_AddString(hList, listOptions[i]);
			if (i == chosenOption)
				ListBox_SetCurSel(hList, i);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			chosenOption = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_LIST_CHOOSE));
			chosenCheck = IsDlgButtonChecked(hwnd, IDC_CHECK_OPTION);
			EndDialog(hwnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;
	}
	return 0;
}

HMODULE GetCurrentFileModule()
{
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(GetCurrentFileModule, &mbi, sizeof(mbi)))
		return (HMODULE)mbi.AllocationBase;
	else
		return NULL;
}

int ChooseList(HWND hwnd, LPCTSTR title, LPCTSTR * options, int cOptions, int nDefault, LPCTSTR pcszCheck, BOOL *retChosen)
{
	listOptions = options;
	countOptions = cOptions;
	dlgTitle = title;
	chosenOption = nDefault;
	dlgCheckText = pcszCheck;
	chosenCheck = *retChosen;
	DialogBox(GetCurrentFileModule(), MAKEINTRESOURCE(IDD_DIALOG_CHOOSE), hwnd, ChooseCallback);
	*retChosen = chosenCheck;
	return chosenOption;
}
