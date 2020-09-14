#pragma once

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

// disable pointer cast warning
#pragma warning(disable: 4311)
//#pragma warning(disable: 4302)

#include <Windows.h>

// debug only
#include <iostream>
#include <bitset>

#define DLG_MEM_SUCCESS 0
#define DLG_MEM_ALLOC_FAILED 1
#define DLG_MEM_FREE_FAILED 2
#define DLG_MEM_NOT_ALIGNED 3
#define DLG_MEM_EXCESS_USAGE 4

/** Dialog Template Overview
 *
 * Start with DLGTEMPLATEEX.
 * Then followed by multiple DLGITEMTEMPLATEEX, specified by cDlgItems, must start at DWORD boundary.
 *
 **/

// DLGITEMTEMPLATEEX Structure, INTERNAL USE ONLY
// Reference: https://docs.microsoft.com/zh-cn/windows/win32/dlgbox/dlgitemtemplateex
// Align: MUST start at DWORD boundary
// Alignment is required for specific fields
typedef struct _item_data {
	DWORD     helpID;
	DWORD     exStyle; // not used in general
	DWORD     style;
	short     x;
	short     y;
	short     cx;
	short     cy;
	DWORD     id;      // identifier

	/* windowClass field: a 16-bit element array
	 * windowClass[0] is 0xFFFF: windowClass[1] is the ordinal value of predefined window class
	 *     0x0080 Button
	 *     0x0081 Edit
	 *     0x0082 Static
	 *     0x0083 List Box
	 *     0x0084 Scroll Bar
	 *     0x0085 Combo Box
	 * otherwise: A UNICODE string whoch is the name of a registered window class
	 *
	 * Align: WORD boundary
	 */
	WCHAR* windowClass;
	size_t size_class;

	/* title field
	 * title[0] is 0xFFFF: title[1] is the ordinal value of additional resource
	 *     (eg. the icon used in icon control)
	 * otherwise: A UNICODE string whoch is the initial text
	 *
	 * Align: WORD boundary
	 */
	WCHAR* title;
	size_t size_title;

	/* extraCount field
	 * Size of additional creation data (in bytes), if not zero, the data is aligned
	 * on next WORD boundary.
	 * Data can be anything.
	 * Data will be sent to dialog through lParam of *WM_CREATE* message.
	 *
	 * Align: WORD boundary
	 */
	WORD      extraCount;
	void*     extraData;

	// linked list - internal use only
	int item_id;
	struct _item_data* prev;
	struct _item_data* next;
} DIALOG_TEMPLATE_ITEM_DATA;

// DLGTEMPLATEEX Structure, INTERNAL USE ONLY
// Reference: https://docs.microsoft.com/zh-cn/windows/win32/dlgbox/dlgtemplateex
// Alignment is required for specific fields: All arrays must be aligned on WORD boundary
typedef struct {
	WORD      dlgVer;    // for extended template, must be 1
	WORD      signature; // for extended template, must be 0xFFFF; standard template otherwise
	DWORD     helpID;
	DWORD     exStyle;   // not used in general
	DWORD     style;
	WORD      cDlgItems;
	short     x;
	short     y;
	short     cx;
	short     cy;
	
	/* menu field: a 16-bit element array 
	 * menu[0] is 0x0000: no menu, no other elements
	 * menu[0] is 0xFFFF: menu[1] is the ordinal value of the menu resource in the executable file
	 * otherwise: A UNICODE string whoch is the name of the menu resource
	 *
	 * Align: WORD boundary
	 */
	WCHAR*    menu;
	size_t    size_menu;

	/* windowClass field: a 16-bit element array
	 * windowClass[0] is 0x0000: use predefined dialog class, no other elements
	 * windowClass[0] is 0xFFFF: windowClass[1] is the ordinal value of predefined window class
	 * otherwise: A UNICODE string whoch is the name of a registered window class
	 *
	 * Align: WORD boundary
	 */
	WCHAR*    windowClass;
	size_t    size_class;

	/* title field
	 * title[0] is 0x0000: no title, no other elements
	 * otherwise: A UNICODE string whoch is the title name
	 *
	 * Align: WORD boundary
	 */
	WCHAR*    title;
	size_t    size_title;

	// !!! Conditional Existing Fields BELOW !!!
	// Following font-related fields ONLY exist when style has DS_SETFONT or DS_SHELLFONT

	WORD      pointsize;
	WORD      weight;
	BYTE      italic;
	BYTE      charset;

	/* typeface field
	 * The name of the typeface for the font.
	 *
	 * Align: WORD boundary
	 */
	WCHAR*    typeface;
	size_t    size_typeface;

	// Item Linked List
	DIALOG_TEMPLATE_ITEM_DATA* pItemBegin;
	DIALOG_TEMPLATE_ITEM_DATA* pItemEnd;
} DIALOG_TEMPLATE_DATA;

PBYTE alignToWORD(PBYTE, int*);
PBYTE alignToDWORD(PBYTE, int*);
int gendlg();
int bindDialogData(PBYTE, PBYTE, DIALOG_TEMPLATE_DATA*, size_t);
size_t getTemplateSizeAligned(DIALOG_TEMPLATE_DATA*);
void dumpDialogData(PBYTE);
INT_PTR DialogDummyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
DIALOG_TEMPLATE_ITEM_DATA* addControl(DIALOG_TEMPLATE_DATA*);
void deleteByReference(DIALOG_TEMPLATE_DATA*, DIALOG_TEMPLATE_ITEM_DATA*);

#endif