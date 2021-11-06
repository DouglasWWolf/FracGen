////////////////////////////////////////////////////////
//  Copyright (c) 2014 Cepheid Corporation
//
//  DisplayBox.cpp
//
//  Implementation for an extended listbox used for display
////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DisplayBox.h"


//============================================================================
// Message map
//============================================================================
BEGIN_MESSAGE_MAP(DisplayBox, CListBox)
END_MESSAGE_MAP()
//============================================================================


//============================================================================
// Display() - Displays a string of a specified color
//============================================================================
void DisplayBox::Display(LPCTSTR lpszItem, int rgb)
{
    AddString(lpszItem, rgb);
    SetCurSel(GetCount()-1);
}
//============================================================================


//============================================================================
// Clear() - Removes all strings from the display-box
//============================================================================
void DisplayBox::Clear()
{
    int i = GetCount();
    while (i) DeleteString(--i);
}
//============================================================================



/////////////////////////////////////////////////////////////////////////////
// DisplayBox message handlers

void DisplayBox::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
{
	if ((int)lpDIS->itemID < 0)
		return; 

	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	COLORREF crText;
	CString sText;
	COLORREF crNorm = (COLORREF)lpDIS->itemData;		// Color information is in item data.
	COLORREF crHilite = RGB(255-GetRValue(crNorm), 255-GetGValue(crNorm), 255-GetBValue(crNorm));


	int nBkMode = pDC->SetBkMode(TRANSPARENT);

	if (lpDIS->itemData)		
	{
			crText = pDC->SetTextColor(crNorm);
	}
	else
	{ 
			crText = pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));	
	}

	GetText(lpDIS->itemID, sText);
	CRect rect = lpDIS->rcItem;

	UINT nFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
	if (GetStyle() & LBS_USETABSTOPS)
		nFormat |= DT_EXPANDTABS;
	
	pDC->DrawText(sText, -1, &rect, nFormat | DT_CALCRECT);
	pDC->DrawText(sText, -1, &rect, nFormat);

	pDC->SetTextColor(crText); 
	pDC->SetBkMode(nBkMode);
	
}
//-------------------------------------------------------------------
void DisplayBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS) 
{
	lpMIS->itemHeight = ::GetSystemMetrics(SM_CYMENUCHECK);	
}

//-------------------------------------------------------------------
//
int DisplayBox::EnableWindow(BOOL bState)
{
	    int nret = ((CListBox*)this)->EnableWindow(bState);
		return nret;
}

//-------------------------------------------------------------------

int DisplayBox::AddString(LPCTSTR lpszItem)
{
    return ((CListBox*)this)->AddString(lpszItem);
}

//-------------------------------------------------------------------
//
int DisplayBox::AddString(LPCTSTR lpszItem, COLORREF rgb)
{
	int nItem = AddString(lpszItem);
	if (nItem >= 0)
		SetItemData(nItem, rgb);

	RedrawWindow();

	return nItem;
}



//-------------------------------------------------------------------
//
int DisplayBox::InsertString(int nIndex, LPCTSTR lpszItem)
{
	return ((CListBox*)this)->InsertString(nIndex, lpszItem);
}	

//-------------------------------------------------------------------
//
int DisplayBox::InsertString(int nIndex, LPCTSTR lpszItem, COLORREF rgb)
{
	int nItem = ((CListBox*)this)->InsertString(nIndex,lpszItem);
	if (nItem >= 0)
		SetItemData(nItem, rgb);

	RedrawWindow();


	return nItem;
}	

//-------------------------------------------------------------------
//
void DisplayBox::SetItemColor(int nIndex, COLORREF rgb)
{
	SetItemData(nIndex, rgb);	
	RedrawWindow();
}	
