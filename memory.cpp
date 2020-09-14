#include "global.hpp"

// Reference: http://bytepointer.com/resources/win32_res_format_older.htm

// aligned to 2-byte boundary
PBYTE alignToWORD(PBYTE ptr, int* padSize)
{
	// multiple of 2 always hold 0 on first binary digit, 
	// and this bit represents the number of padding bits (0 or 1)
	unsigned long long val = (unsigned long long)ptr & 1;

	if (padSize) {
		*padSize = val;
	}

	return ptr + val;
}

// aligned to 4-byte boundary
PBYTE alignToDWORD(PBYTE ptr, int* padSize)
{
	// standard padding (on n-byte bound): aligned = ((addr + n - 1) / n) * n, 
	// where the division only keep integer part
	// unsigned long long (hence __int64) is the only type that is safe for casting pointer
	unsigned long long val = (((unsigned long long)ptr + 3) >> 2) << 2;

	if (padSize) {
		*padSize = (PBYTE)val - ptr;
	}

	return (PBYTE)val;
}

size_t getTemplateSizeAligned(DIALOG_TEMPLATE_DATA* pdtd)
{
	// set initial dummy addr to 0 since it is on DWORD boundary
	PBYTE s = (PBYTE)0; // use a false pointer to test size after padding

	s += 26; // first 10 fields

	s = alignToWORD(s, NULL);
	s += pdtd->size_class;
	s = alignToWORD(s, NULL);
	s += pdtd->size_menu;
	s = alignToWORD(s, NULL);
	s += pdtd->size_title;
	
	if (pdtd->style & DS_SETFONT || pdtd->style & DS_SHELLFONT) {
		s += 6; // 4 fields for fonts
		s = alignToWORD(s, NULL);
		s += pdtd->size_typeface;
	}

	if (pdtd->cDlgItems != 0) {
		DIALOG_TEMPLATE_ITEM_DATA* pItem = pdtd->pItemBegin;
		while (pItem) {
			s = alignToDWORD(s, NULL);

			s += 24; // 8 fields
			s = alignToWORD(s, NULL);
			s += pItem->size_class;
			s = alignToWORD(s, NULL);
			s += pItem->size_title;
			s += 2;

			if (pItem->extraCount > 0) {
				s = alignToWORD(s, NULL);
				s += pItem->extraCount;
			}

			pItem = pItem->next;
		}
	}

	return (size_t)s;
}

// Every array is WCHAR and WORD-aligned
bool safeCopy(PBYTE pBlockStart, PBYTE pTempl, void* data, size_t size, size_t reserved)
{
	pTempl = alignToWORD(pTempl, NULL);

	if (pTempl + size - pBlockStart > reserved) {
		return false;
	}

	memcpy(pTempl, data, size);

	return true;
}

int bindDialogData(PBYTE pMem, PBYTE pTempl, DIALOG_TEMPLATE_DATA* pdtd, size_t reserved)
{
	size_t delta;

	// if not aligned on DWORD boundary, raise error
	if ((unsigned long long)pTempl & 3) {
		return DLG_MEM_NOT_ALIGNED;
	}
	
	*((PWORD)pTempl) = pdtd->dlgVer;
	pTempl += sizeof(WORD);
	*((PWORD)pTempl) = pdtd->signature;
	pTempl += sizeof(WORD);
	*((PDWORD)pTempl) = pdtd->helpID;
	pTempl += sizeof(DWORD);
	*((PDWORD)pTempl) = pdtd->exStyle;
	pTempl += sizeof(DWORD);
	*((PDWORD)pTempl) = pdtd->style;
	pTempl += sizeof(DWORD);
	*((PWORD)pTempl) = pdtd->cDlgItems;
	pTempl += sizeof(WORD);
	*((short*)pTempl) = pdtd->x;
	pTempl += sizeof(short);
	*((short*)pTempl) = pdtd->y;
	pTempl += sizeof(short);
	*((short*)pTempl) = pdtd->cx;
	pTempl += sizeof(short);
	*((short*)pTempl) = pdtd->cy;
	pTempl += sizeof(short);
	
	delta = pdtd->size_menu;
	if (!safeCopy(pMem, pTempl, pdtd->menu, delta, reserved)) {
		return DLG_MEM_EXCESS_USAGE;
	}
	pTempl += delta;

	delta = pdtd->size_class;
	if (!safeCopy(pMem, pTempl, pdtd->windowClass, delta, reserved)) {
		return DLG_MEM_EXCESS_USAGE;
	}
	pTempl += delta;

	delta = pdtd->size_title;
	if (!safeCopy(pMem, pTempl, pdtd->title, delta, reserved)) {
		return DLG_MEM_EXCESS_USAGE;
	}
	pTempl += delta;

	if (pdtd->style & DS_SETFONT || pdtd->style & DS_SHELLFONT)
	{
		*((PWORD)pTempl) = pdtd->pointsize;
		pTempl += sizeof(WORD);
		*((PWORD)pTempl) = pdtd->weight;
		pTempl += sizeof(WORD);
		*pTempl = pdtd->italic;
		pTempl += sizeof(BYTE);
		*pTempl = pdtd->charset;
		pTempl += sizeof(BYTE);

		delta = pdtd->size_typeface;
		if (!safeCopy(pMem, pTempl, pdtd->typeface, delta, reserved)) {
			return DLG_MEM_EXCESS_USAGE;
		}
		pTempl += delta;
	}

	if (pdtd->cDlgItems > 0)
	{
		DIALOG_TEMPLATE_ITEM_DATA* pItem = pdtd->pItemBegin;
		while (pItem) {
			pTempl = alignToDWORD(pTempl, NULL);

			*((PDWORD)pTempl) = pItem->helpID;
			pTempl += sizeof(DWORD);
			*((PDWORD)pTempl) = pItem->exStyle;
			pTempl += sizeof(DWORD);
			*((PDWORD)pTempl) = pItem->style;
			pTempl += sizeof(DWORD);
			*((short*)pTempl) = pItem->x;
			pTempl += sizeof(short);
			*((short*)pTempl) = pItem->y;
			pTempl += sizeof(short);
			*((short*)pTempl) = pItem->cx;
			pTempl += sizeof(short);
			*((short*)pTempl) = pItem->cy;
			pTempl += sizeof(short);
			*((PDWORD)pTempl) = pItem->id;
			pTempl += sizeof(DWORD);

			delta = pItem->size_class;
			if (!safeCopy(pMem, pTempl, pItem->windowClass, delta, reserved)) {
				return DLG_MEM_EXCESS_USAGE;
			}
			pTempl += delta;

			delta = pItem->size_title;
			if (!safeCopy(pMem, pTempl, pItem->title, delta, reserved)) {
				return DLG_MEM_EXCESS_USAGE;
			}
			pTempl += delta;

			*((PWORD)pTempl) = pItem->extraCount;
			pTempl += sizeof(WORD);

			if (pItem->extraCount > 0) {
				if (!safeCopy(pMem, pTempl, pItem->extraData, pItem->extraCount, reserved)) {
					return DLG_MEM_EXCESS_USAGE;
				}
				pTempl += pItem->extraCount;
			}

			pItem = pItem->next;
		}
	}

	return DLG_MEM_SUCCESS;
}

void dumpDialogData(PBYTE pStart)
{
	int padding;
	DWORD style;
	WORD cItem;

	std::cout << "Dialog Version: " << *(PWORD)pStart << std::endl;
	pStart += sizeof(WORD);
	std::cout << "Signature: " << std::hex << *(PWORD)pStart << std::dec << std::endl;
	pStart += sizeof(WORD);
	std::cout << "Help ID: " << *(PDWORD)pStart << std::endl;
	pStart += sizeof(DWORD);
	std::cout << "Extended Style: " << std::hex << *(PDWORD)pStart << std::dec << std::endl;
	pStart += sizeof(DWORD);
	style = *(PDWORD)pStart;
	std::cout << "Style: " << std::hex << style << std::dec << std::endl;
	pStart += sizeof(DWORD);
	cItem = *(PWORD)pStart;
	std::cout << "Dialog Item Count: " << cItem << std::endl;
	pStart += sizeof(WORD);
	std::cout << "x: " << *(short*)pStart << std::endl;
	pStart += sizeof(short);
	std::cout << "y: " << *(short*)pStart << std::endl;
	pStart += sizeof(short);
	std::cout << "Width: " << *(short*)pStart << std::endl;
	pStart += sizeof(short);
	std::cout << "Height: " << *(short*)pStart << std::endl;
	pStart += sizeof(short);

	pStart = alignToWORD(pStart, &padding);
	std::cout << "Menu (with padding " << padding << " bytes): ";
	if (*(WCHAR*)pStart == 0xFFFF) {
		pStart += sizeof(WCHAR);
		std::cout << "Ord(" << std::hex << (int)(*(PWCHAR)pStart) << ")" << std::dec << std::endl;
	}
	else if (*(WCHAR*)pStart == 0x0000) {
		std::cout << "N/A\n";
	}
	else {
		while (*(WCHAR*)pStart != 0x0000) {
			std::cout << *(PWCHAR)pStart;
			pStart += sizeof(WCHAR);
		}
		std::cout << std::endl;
	}
	pStart += sizeof(WCHAR);

	pStart = alignToWORD(pStart, &padding);
	std::cout << "Window Class (with padding " << padding << " bytes): ";
	if (*(WCHAR*)pStart == 0xFFFF) {
		pStart += sizeof(WCHAR);
		std::cout << "Ord(" << std::hex << (int)(*(PWCHAR)pStart) << ")" << std::dec << std::endl;
	}
	else if (*(WCHAR*)pStart == 0x0000) {
		std::cout << "Use pre-defined dialog class\n";
	}
	else {
		while (*(WCHAR*)pStart != 0x0000) {
			std::cout << *(PWCHAR)pStart;
			pStart += sizeof(WCHAR);
		}
		std::cout << std::endl;
	}
	pStart += sizeof(WCHAR);

	pStart = alignToWORD(pStart, &padding);
	std::cout << "Title (with padding " << padding << " bytes): ";
	if (*(WCHAR*)pStart == 0x0000) {
		std::cout << "N/A\n";
	}
	else {
		while (*(WCHAR*)pStart != 0x0000) {
			std::wcout << (WCHAR)(*(WCHAR*)pStart);
			pStart += sizeof(WCHAR);
		}
		std::cout << std::endl;
	}
	pStart += sizeof(WCHAR);

	if (style & DS_SETFONT || style & DS_SHELLFONT) {
		std::cout << "\nFont configuration:\n";

		std::cout << "    Point Size: " << *(PWORD)pStart << std::endl;
		pStart += sizeof(WORD);
		std::cout << "    Weight: " << *(PWORD)pStart << std::endl;
		pStart += sizeof(WORD);
		std::cout << "    Is Italic: " << (*pStart ? "Yes" : "No") << std::endl;
		pStart++;
		std::cout << "    Charset: " << std::hex << (int)*pStart << std::dec << std::endl;
		pStart++;

		pStart = alignToWORD(pStart, &padding);
		std::cout << "    Typeface (with padding " << padding << " bytes): ";
		while (*(WCHAR*)pStart != 0x0000) {
			std::wcout << (WCHAR)(*(WCHAR*)pStart);
			pStart += sizeof(WCHAR);
		}
		std::cout << std::endl;
		pStart += sizeof(WCHAR); // skip null-terminate character
	}
	else {
		std::cout << "\nNo additional font configurations.\n";
	}

	if (cItem != 0) {
		for (int i = 1; i <= cItem; i++) {
			pStart = alignToDWORD(pStart, &padding);
			std::cout << "\nControl " << i << " (padding: " << padding << " bytes)\n";

			std::cout << "    Help ID: " << *(PDWORD)pStart << std::endl;
			pStart += sizeof(DWORD);
			std::cout << "    Extended Style: " << std::hex << *(PDWORD)pStart << std::dec << std::endl;
			pStart += sizeof(DWORD);
			std::cout << "    Style: " << std::hex << *(PDWORD)pStart << std::dec << std::endl;
			pStart += sizeof(DWORD);
			std::cout << "    x: " << *(short*)pStart << std::endl;
			pStart += sizeof(short);
			std::cout << "    y: " << *(short*)pStart << std::endl;
			pStart += sizeof(short);
			std::cout << "    Width: " << *(short*)pStart << std::endl;
			pStart += sizeof(short);
			std::cout << "    Height: " << *(short*)pStart << std::endl;
			pStart += sizeof(short);
			std::cout << "    ID: " << *(PDWORD)pStart << std::endl;
			pStart += sizeof(DWORD);

			pStart = alignToWORD(pStart, &padding);
			std::cout << "    Class Name (with padding " << padding << " bytes): ";
			if (*(WCHAR*)pStart == 0xFFFF) {
				pStart += sizeof(WCHAR);
				WCHAR cls = *(PWCHAR)pStart;
				switch (cls) {
					case 0x0080:
						std::cout << "Button (0x";
						break;
					case 0x0081:
						std::cout << "Edit (0x";
						break;
					case 0x0082:
						std::cout << "Static (0x";
						break;
					case 0x0083:
						std::cout << "List Box (0x";
						break;
					case 0x0084:
						std::cout << "Scroll Bar (0x";
						break;
					case 0x0085:
						std::cout << "Combo Box (0x";
						break;
					default:
						std::cout << "Unknown Control Type (0x";
						break;
				}
				std::cout << std::hex << cls << ")" << std::dec << std::endl;
			}
			else {
				while (*(WCHAR*)pStart != 0x0000) {
					std::wcout << (WCHAR)(*(WCHAR*)pStart);
					pStart += sizeof(WCHAR);
				}
				std::cout << std::endl;
			}
			pStart += sizeof(WCHAR);

			pStart = alignToWORD(pStart, &padding);
			std::cout << "    Title (with padding " << padding << " bytes): ";
			if (*(WCHAR*)pStart == 0xFFFF) {
				pStart += sizeof(WCHAR);
				std::cout << "Ord(" << std::hex << (int)(*(PWCHAR)pStart) << ")" << std::dec << std::endl;
			}
			else {
				while (*(WCHAR*)pStart != 0x0000) {
					std::wcout << (WCHAR)(*(WCHAR*)pStart);
					pStart += sizeof(WCHAR);
				}
				std::cout << std::endl;
			}
			pStart += sizeof(WCHAR);
			
			WORD cds = *(PWORD)pStart;
			std::cout << "    Creation Data Size: " << cds << " bytes\n";
			pStart += sizeof(WORD);

			if (cds > 0) {
				pStart = alignToWORD(pStart, &padding);
				std::cout << "        Data start at " 
					<< std::hex << (unsigned long long)pStart << std::dec 
					<< ", padding " << padding << " byte" << std::endl;
				pStart += cds;
			}
		}
	}
	else {
		std::cout << "No control in dialog.\n";
	}

	std::cout << std::endl;
}
