#include <Windows.h>
#include "global.hpp"

// Reference: 
// https://docs.microsoft.com/en-us/windows/win32/dlgbox/using-dialog-boxes#creating-a-template-in-memory

INT_PTR DialogDummyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;
		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == 102) {
				// test button click test
				EndDialog(hwnd, 0);
			}
			break;
		default:
			break;
	}

	return FALSE;
}

DIALOG_TEMPLATE_DATA* createTemplate()
{
	DIALOG_TEMPLATE_DATA* pdtd = (DIALOG_TEMPLATE_DATA*)malloc(sizeof(DIALOG_TEMPLATE_DATA));

	memset(pdtd, 0, sizeof(DIALOG_TEMPLATE_DATA));
	pdtd->dlgVer = 1;
	pdtd->signature = 0xFFFF;
	pdtd->cDlgItems = 0;

	return pdtd;
}

void releaseTemplate(DIALOG_TEMPLATE_DATA* pdtd)
{
	free(pdtd->menu);
	free(pdtd->windowClass);
	free(pdtd->title);

	DIALOG_TEMPLATE_ITEM_DATA* pItem = pdtd->pItemBegin, * pTemp;
	while (pItem) {
		pTemp = pItem->next;
		deleteByReference(pdtd, pItem);
		pItem = pTemp;
	}

	free(pdtd);
}

DIALOG_TEMPLATE_DATA* generateTemplate()
{
	DIALOG_TEMPLATE_DATA* pdtd = createTemplate();
	pdtd->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION;
	pdtd->x = 50;
	pdtd->y = 50;
	pdtd->cx = 200;
	pdtd->cy = 100;
	// menu
	pdtd->size_menu = sizeof(WCHAR) * 1;
	pdtd->menu = (WCHAR*)malloc(pdtd->size_menu);
	pdtd->menu[0] = 0x0000;
	// class
	pdtd->size_class = sizeof(WCHAR) * 1;
	pdtd->windowClass = (WCHAR*)malloc(pdtd->size_class);
	pdtd->windowClass[0] = 0x0000;
	// title
	WCHAR tc[] = L"Hello";
	pdtd->size_title = sizeof(WCHAR) * (wcslen(tc) + 1);
	pdtd->title = (WCHAR*)malloc(pdtd->size_title);
	memcpy(pdtd->title, tc, pdtd->size_title);
	// font
	// eg. FONT 8, "MS Shell Dlg", 400, 0, 0x0
	pdtd->style |= DS_SETFONT;
	pdtd->pointsize = 8;
	pdtd->weight = 400;
	pdtd->italic = 0;
	pdtd->charset = 0x0;
	WCHAR fontName[] = L"MS Shell Dlg";
	pdtd->size_typeface = sizeof(WCHAR) * (wcslen(fontName) + 1);
	pdtd->typeface = (WCHAR*)malloc(pdtd->size_typeface);
	memcpy(pdtd->typeface, fontName, pdtd->size_typeface);

	DIALOG_TEMPLATE_ITEM_DATA* pItem = addControl(pdtd);
	pItem->style = SS_LEFT | WS_GROUP | WS_VISIBLE;
	pItem->x = 0;
	pItem->y = 0;
	pItem->cx = 200;
	pItem->cy = 10;
	pItem->id = 101;
	// class
	pItem->size_class = sizeof(WCHAR) * 2;
	pItem->windowClass = (WCHAR*)malloc(pItem->size_class);
	pItem->windowClass[0] = 0xFFFF;
	pItem->windowClass[1] = 0x0082;
	// title
	WCHAR sTest[] = L"This is test control text";
	pItem->size_title = sizeof(WCHAR) * (wcslen(sTest) + 1);
	pItem->title = (WCHAR*)malloc(pItem->size_title);
	memcpy(pItem->title, sTest, pItem->size_title);
	// creation data
	pItem->extraCount = 0;

	pItem = addControl(pdtd);
	pItem->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;
	pItem->x = 0;
	pItem->y = 30;
	pItem->cx = 50;
	pItem->cy = 15;
	pItem->id = 102;
	pItem->size_class = sizeof(WCHAR) * 2;
	pItem->windowClass = (WCHAR*)malloc(pItem->size_class);
	pItem->windowClass[0] = 0xFFFF;
	pItem->windowClass[1] = 0x0080;
	WCHAR sBtn[] = L"Exit";
	pItem->size_title = sizeof(WCHAR) * (wcslen(sBtn) + 1);
	pItem->title = (WCHAR*)malloc(pItem->size_title);
	memcpy(pItem->title, sBtn, pItem->size_title);
	pItem->extraCount = 0;

	return pdtd;
}

int gendlg()
{
	DIALOG_TEMPLATE_DATA* pdtd = generateTemplate();

	// we need few bytes guard the boundary because the pointer might not align on DWORD boundary
	size_t templateSize = getTemplateSizeAligned(pdtd) + 4;
	PBYTE pMem = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, templateSize);
	if (!pMem) {
		releaseTemplate(pdtd);
		return DLG_MEM_ALLOC_FAILED;
	}

	// Align start of the template to DWORD boundary to make sure 
	// any padding is consistent in both memory and file.
	PBYTE pTempl = alignToDWORD(pMem, NULL);
	int result = bindDialogData(pMem, pTempl, pdtd, templateSize);
	releaseTemplate(pdtd);

	std::cout << "Memory block start at: " << std::hex << (int)pMem << std::dec << std::endl;
	std::cout << "Template start at: " << std::hex << (int)pTempl << std::dec << std::endl;
	std::cout << "Template size: " << templateSize << " bytes\n" << std::endl;

	if (result == DLG_MEM_SUCCESS)
	{
		dumpDialogData(pTempl);

		// Do NOT use CreateDialogIndirectParam which creates modless dialog which will exit immediately
		// since the main thread will not wait for it.
		// Need modal dialog box
		INT_PTR h = DialogBoxIndirectParam(GetModuleHandle(NULL), (LPCDLGTEMPLATE)pTempl, NULL, (DLGPROC)DialogDummyProc, 0);
		if (h == -1) {
			std::cout << "Error: " << GetLastError() << std::endl;
		}
		else {
			std::cout << "Test completed.\n";
		}
	}
	else if (result == DLG_MEM_NOT_ALIGNED) {
		std::cout << "Template memory mis-aligned on 4-byte boundary.\n";
	}
	else if (result == DLG_MEM_EXCESS_USAGE) {
		std::cout << "Template memory corrupted, data is out of boundary.\n";
	}

	if (!HeapFree(GetProcessHeap(), 0, pMem)) {
		return DLG_MEM_FREE_FAILED;
	}
	return result;
}
