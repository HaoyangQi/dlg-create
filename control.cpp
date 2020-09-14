#include "global.hpp"

static int item_id_count = 0;

void enqueControlItem(DIALOG_TEMPLATE_DATA* pdtd, DIALOG_TEMPLATE_ITEM_DATA* pItem)
{
	pItem->prev = NULL;
	if (pdtd->pItemBegin == NULL) {
		pItem->next = NULL;
		pdtd->pItemBegin = pItem;
		pdtd->pItemEnd = pItem;
	}
	else {
		pItem->next = pdtd->pItemBegin;
		pdtd->pItemBegin = pItem;
	}
}

DIALOG_TEMPLATE_ITEM_DATA* findByItemID(DIALOG_TEMPLATE_DATA* pdtd, int id)
{
	DIALOG_TEMPLATE_ITEM_DATA* pItem = pdtd->pItemBegin;

	while (pItem) {
		if (pItem->item_id == id) {
			return pItem;
		}
		pItem = pItem->next;
	}

	return NULL;
}

DIALOG_TEMPLATE_ITEM_DATA* findByControlID(DIALOG_TEMPLATE_DATA* pdtd, int id)
{
	DIALOG_TEMPLATE_ITEM_DATA* pItem = pdtd->pItemBegin;

	while (pItem) {
		if (pItem->id == id) {
			return pItem;
		}
		pItem = pItem->next;
	}

	return NULL;
}

bool safeDeleteControlItem(DIALOG_TEMPLATE_ITEM_DATA* pItem)
{
	if (!pItem || pItem->prev || pItem->next) {
		return false;
	}

	if (pItem->windowClass) {
		free(pItem->windowClass);
		pItem->windowClass = NULL;
	}

	if (pItem->title) {
		free(pItem->title);
		pItem->title = NULL;
	}

	if (pItem->extraData) {
		free(pItem->extraData);
		pItem->extraData = NULL;
	}

	free(pItem);
	return true;
}

void deleteByReference(DIALOG_TEMPLATE_DATA* pdtd, DIALOG_TEMPLATE_ITEM_DATA* pItem)
{
	if (pItem->prev) {
		pItem->prev->next = pItem->next;
	}
	else {
		pdtd->pItemBegin = pItem->next;
	}

	if (pItem->next) {
		pItem->next->prev = pItem->prev;
	}
	else {
		pdtd->pItemEnd = pItem->prev;
	}

	pItem->prev = NULL;
	pItem->next = NULL;
	if (safeDeleteControlItem(pItem)) {
		pdtd->cDlgItems--;
	}
}

void deleteByItemID(DIALOG_TEMPLATE_DATA* pdtd, int id)
{
	deleteByReference(pdtd, findByItemID(pdtd, id));
}

void deleteByControlID(DIALOG_TEMPLATE_DATA* pdtd, int id)
{
	deleteByReference(pdtd, findByControlID(pdtd, id));
}

DIALOG_TEMPLATE_ITEM_DATA* addControl(DIALOG_TEMPLATE_DATA* pdtd)
{
	DIALOG_TEMPLATE_ITEM_DATA* pItem = (DIALOG_TEMPLATE_ITEM_DATA*)malloc(sizeof(DIALOG_TEMPLATE_ITEM_DATA));

	memset(pItem, 0, sizeof(DIALOG_TEMPLATE_ITEM_DATA));
	pItem->item_id = item_id_count++;
	enqueControlItem(pdtd, pItem);
	pdtd->cDlgItems++;

	return pItem;
}
