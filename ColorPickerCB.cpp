/*|*\
|*|  File:      ColorPickerCB.cpp
|*|  By:        James R. Twine
|*|             Copyright 1998, James R. Twine
|*|             Copyright 1999-2000, TransactionWorks, Inc.
|*|             Ported to pure Win32 (no MFC).
\*|*/

#include "stdafx.h"
#include "ColorPickerCB.h"

//
//	Load/Create Standard Colors...
//
static SColorAndName g_pColors[] =
{
	SColorAndName( RGB( 0xF0, 0xF8, 0xFF ), L"AliceBlue" ),
	SColorAndName( RGB( 0xFA, 0xEB, 0xD7 ), L"AntiqueWhite" ),
	SColorAndName( RGB( 0x00, 0xFF, 0xFF ), L"Aqua" ),
	SColorAndName( RGB( 0x7F, 0xFF, 0xD4 ), L"Aquamarine" ),
	SColorAndName( RGB( 0xF0, 0xFF, 0xFF ), L"Azure" ),
	SColorAndName( RGB( 0xF5, 0xF5, 0xDC ), L"Beige" ),
	SColorAndName( RGB( 0xFF, 0xE4, 0xC4 ), L"Bisque" ),
	SColorAndName( RGB( 0x00, 0x00, 0x00 ), L"Black" ),
	SColorAndName( RGB( 0xFF, 0xEB, 0xCD ), L"BlanchedAlmond" ),
	SColorAndName( RGB( 0x00, 0x00, 0xFF ), L"Blue" ),
	SColorAndName( RGB( 0x8A, 0x2B, 0xE2 ), L"BlueViolet" ),
	SColorAndName( RGB( 0xA5, 0x2A, 0x2A ), L"Brown" ),
	SColorAndName( RGB( 0xDE, 0xB8, 0x87 ), L"BurlyWood" ),
	SColorAndName( RGB( 0x5F, 0x9E, 0xA0 ), L"CadetBlue" ),
	SColorAndName( RGB( 0x7F, 0xFF, 0x00 ), L"Chartreuse" ),
	SColorAndName( RGB( 0xD2, 0x69, 0x1E ), L"Chocolate" ),
	SColorAndName( RGB( 0xFF, 0x7F, 0x50 ), L"Coral" ),
	SColorAndName( RGB( 0x64, 0x95, 0xED ), L"CornflowerBlue" ),
	SColorAndName( RGB( 0xFF, 0xF8, 0xDC ), L"Cornsilk" ),
	SColorAndName( RGB( 0xDC, 0x14, 0x3C ), L"Crimson" ),
	SColorAndName( RGB( 0x00, 0xFF, 0xFF ), L"Cyan" ),
	SColorAndName( RGB( 0x00, 0x00, 0x8B ), L"DarkBlue" ),
	SColorAndName( RGB( 0x00, 0x8B, 0x8B ), L"DarkCyan" ),
	SColorAndName( RGB( 0xB8, 0x86, 0x0B ), L"DarkGoldenrod" ),
	SColorAndName( RGB( 0xA9, 0xA9, 0xA9 ), L"DarkGray" ),
	SColorAndName( RGB( 0x00, 0x64, 0x00 ), L"DarkGreen" ),
	SColorAndName( RGB( 0xBD, 0xB7, 0x6B ), L"DarkKhaki" ),
	SColorAndName( RGB( 0x8B, 0x00, 0x8B ), L"DarkMagenta" ),
	SColorAndName( RGB( 0x55, 0x6B, 0x2F ), L"DarkOliveGreen" ),
	SColorAndName( RGB( 0xFF, 0x8C, 0x00 ), L"DarkOrange" ),
	SColorAndName( RGB( 0x99, 0x32, 0xCC ), L"DarkOrchid" ),
	SColorAndName( RGB( 0x8B, 0x00, 0x00 ), L"DarkRed" ),
	SColorAndName( RGB( 0xE9, 0x96, 0x7A ), L"DarkSalmon" ),
	SColorAndName( RGB( 0x8F, 0xBC, 0x8F ), L"DarkSeaGreen" ),
	SColorAndName( RGB( 0x48, 0x3D, 0x8B ), L"DarkSlateBlue" ),
	SColorAndName( RGB( 0x2F, 0x4F, 0x4F ), L"DarkSlateGray" ),
	SColorAndName( RGB( 0x00, 0xCE, 0xD1 ), L"DarkTurquoise" ),
	SColorAndName( RGB( 0x94, 0x00, 0xD3 ), L"DarkViolet" ),
	SColorAndName( RGB( 0xFF, 0x14, 0x93 ), L"DeepPink" ),
	SColorAndName( RGB( 0x00, 0xBF, 0xFF ), L"DeepSkyBlue" ),
	SColorAndName( RGB( 0x69, 0x69, 0x69 ), L"DimGray" ),
	SColorAndName( RGB( 0x1E, 0x90, 0xFF ), L"DodgerBlue" ),
	SColorAndName( RGB( 0xB2, 0x22, 0x22 ), L"FireBrick" ),
	SColorAndName( RGB( 0xFF, 0xFA, 0xF0 ), L"FloralWhite" ),
	SColorAndName( RGB( 0x22, 0x8B, 0x22 ), L"ForestGreen" ),
	SColorAndName( RGB( 0xFF, 0x00, 0xFF ), L"Fuchsia" ),
	SColorAndName( RGB( 0xDC, 0xDC, 0xDC ), L"Gainsboro" ),
	SColorAndName( RGB( 0xF8, 0xF8, 0xFF ), L"GhostWhite" ),
	SColorAndName( RGB( 0xFF, 0xD7, 0x00 ), L"Gold" ),
	SColorAndName( RGB( 0xDA, 0xA5, 0x20 ), L"Goldenrod" ),
	SColorAndName( RGB( 0x80, 0x80, 0x80 ), L"Gray" ),
	SColorAndName( RGB( 0x00, 0x80, 0x00 ), L"Green" ),
	SColorAndName( RGB( 0xAD, 0xFF, 0x2F ), L"GreenYellow" ),
	SColorAndName( RGB( 0xF0, 0xFF, 0xF0 ), L"Honeydew" ),
	SColorAndName( RGB( 0xFF, 0x69, 0xB4 ), L"HotPink" ),
	SColorAndName( RGB( 0xCD, 0x5C, 0x5C ), L"IndianRed" ),
	SColorAndName( RGB( 0x4B, 0x00, 0x82 ), L"Indigo" ),
	SColorAndName( RGB( 0xFF, 0xFF, 0xF0 ), L"Ivory" ),
	SColorAndName( RGB( 0xF0, 0xE6, 0x8C ), L"Khaki" ),
	SColorAndName( RGB( 0xE6, 0xE6, 0xFA ), L"Lavender" ),
	SColorAndName( RGB( 0xFF, 0xF0, 0xF5 ), L"LavenderBlush" ),
	SColorAndName( RGB( 0x7C, 0xFC, 0x00 ), L"LawnGreen" ),
	SColorAndName( RGB( 0xFF, 0xFA, 0xCD ), L"LemonChiffon" ),
	SColorAndName( RGB( 0xAD, 0xD8, 0xE6 ), L"LightBlue" ),
	SColorAndName( RGB( 0xF0, 0x80, 0x80 ), L"LightCoral" ),
	SColorAndName( RGB( 0xE0, 0xFF, 0xFF ), L"LightCyan" ),
	SColorAndName( RGB( 0xFA, 0xFA, 0xD2 ), L"LightGoldenrodYellow" ),
	SColorAndName( RGB( 0x90, 0xEE, 0x90 ), L"LightGreen" ),
	SColorAndName( RGB( 0xD3, 0xD3, 0xD3 ), L"LightGrey" ),
	SColorAndName( RGB( 0xFF, 0xB6, 0xC1 ), L"LightPink" ),
	SColorAndName( RGB( 0xFF, 0xA0, 0x7A ), L"LightSalmon" ),
	SColorAndName( RGB( 0x20, 0xB2, 0xAA ), L"LightSeaGreen" ),
	SColorAndName( RGB( 0x87, 0xCE, 0xFA ), L"LightSkyBlue" ),
	SColorAndName( RGB( 0x77, 0x88, 0x99 ), L"LightSlateGray" ),
	SColorAndName( RGB( 0xB0, 0xC4, 0xDE ), L"LightSteelBlue" ),
	SColorAndName( RGB( 0xFF, 0xFF, 0xE0 ), L"LightYellow" ),
	SColorAndName( RGB( 0x00, 0xFF, 0x00 ), L"Lime" ),
	SColorAndName( RGB( 0x32, 0xCD, 0x32 ), L"LimeGreen" ),
	SColorAndName( RGB( 0xFA, 0xF0, 0xE6 ), L"Linen" ),
	SColorAndName( RGB( 0xFF, 0x00, 0xFF ), L"Magenta" ),
	SColorAndName( RGB( 0x80, 0x00, 0x00 ), L"Maroon" ),
	SColorAndName( RGB( 0x66, 0xCD, 0xAA ), L"MediumAquamarine" ),
	SColorAndName( RGB( 0x00, 0x00, 0xCD ), L"MediumBlue" ),
	SColorAndName( RGB( 0xBA, 0x55, 0xD3 ), L"MediumOrchid" ),
	SColorAndName( RGB( 0x93, 0x70, 0xDB ), L"MediumPurple" ),
	SColorAndName( RGB( 0x3C, 0xB3, 0x71 ), L"MediumSeaGreen" ),
	SColorAndName( RGB( 0x7B, 0x68, 0xEE ), L"MediumSlateBlue" ),
	SColorAndName( RGB( 0x00, 0xFA, 0x9A ), L"MediumSpringGreen" ),
	SColorAndName( RGB( 0x48, 0xD1, 0xCC ), L"MediumTurquoise" ),
	SColorAndName( RGB( 0xC7, 0x15, 0x85 ), L"MediumVioletRed" ),
	SColorAndName( RGB( 0x19, 0x19, 0x70 ), L"MidnightBlue" ),
	SColorAndName( RGB( 0xF5, 0xFF, 0xFA ), L"MintCream" ),
	SColorAndName( RGB( 0xFF, 0xE4, 0xE1 ), L"MistyRose" ),
	SColorAndName( RGB( 0xFF, 0xE4, 0xB5 ), L"Moccasin" ),
	SColorAndName( RGB( 0xFF, 0xDE, 0xAD ), L"NavajoWhite" ),
	SColorAndName( RGB( 0x00, 0x00, 0x80 ), L"Navy" ),
	SColorAndName( RGB( 0xFD, 0xF5, 0xE6 ), L"OldLace" ),
	SColorAndName( RGB( 0x80, 0x80, 0x00 ), L"Olive" ),
	SColorAndName( RGB( 0x6B, 0x8E, 0x23 ), L"OliveDrab" ),
	SColorAndName( RGB( 0xFF, 0xA5, 0x00 ), L"Orange" ),
	SColorAndName( RGB( 0xFF, 0x45, 0x00 ), L"OrangeRed" ),
	SColorAndName( RGB( 0xDA, 0x70, 0xD6 ), L"Orchid" ),
	SColorAndName( RGB( 0xEE, 0xE8, 0xAA ), L"PaleGoldenrod" ),
	SColorAndName( RGB( 0x98, 0xFB, 0x98 ), L"PaleGreen" ),
	SColorAndName( RGB( 0xAF, 0xEE, 0xEE ), L"PaleTurquoise" ),
	SColorAndName( RGB( 0xDB, 0x70, 0x93 ), L"PaleVioletRed" ),
	SColorAndName( RGB( 0xFF, 0xEF, 0xD5 ), L"PapayaWhip" ),
	SColorAndName( RGB( 0xFF, 0xDA, 0xB9 ), L"PeachPuff" ),
	SColorAndName( RGB( 0xCD, 0x85, 0x3F ), L"Peru" ),
	SColorAndName( RGB( 0xFF, 0xC0, 0xCB ), L"Pink" ),
	SColorAndName( RGB( 0xDD, 0xA0, 0xDD ), L"Plum" ),
	SColorAndName( RGB( 0xB0, 0xE0, 0xE6 ), L"PowderBlue" ),
	SColorAndName( RGB( 0x80, 0x00, 0x80 ), L"Purple" ),
	SColorAndName( RGB( 0xFF, 0x00, 0x00 ), L"Red" ),
	SColorAndName( RGB( 0xBC, 0x8F, 0x8F ), L"RosyBrown" ),
	SColorAndName( RGB( 0x41, 0x69, 0xE1 ), L"RoyalBlue" ),
	SColorAndName( RGB( 0x8B, 0x45, 0x13 ), L"SaddleBrown" ),
	SColorAndName( RGB( 0xFA, 0x80, 0x72 ), L"Salmon" ),
	SColorAndName( RGB( 0xF4, 0xA4, 0x60 ), L"SandyBrown" ),
	SColorAndName( RGB( 0x2E, 0x8B, 0x57 ), L"SeaGreen" ),
	SColorAndName( RGB( 0xFF, 0xF5, 0xEE ), L"Seashell" ),
	SColorAndName( RGB( 0xA0, 0x52, 0x2D ), L"Sienna" ),
	SColorAndName( RGB( 0xC0, 0xC0, 0xC0 ), L"Silver" ),
	SColorAndName( RGB( 0x87, 0xCE, 0xEB ), L"SkyBlue" ),
	SColorAndName( RGB( 0x6A, 0x5A, 0xCD ), L"SlateBlue" ),
	SColorAndName( RGB( 0x70, 0x80, 0x90 ), L"SlateGray" ),
	SColorAndName( RGB( 0xFF, 0xFA, 0xFA ), L"Snow" ),
	SColorAndName( RGB( 0x00, 0xFF, 0x7F ), L"SpringGreen" ),
	SColorAndName( RGB( 0xF0, 0xF0, 0xF0 ), L"Standard back" ),
	SColorAndName( RGB( 0x46, 0x82, 0xB4 ), L"SteelBlue" ),
	SColorAndName( RGB( 0xD2, 0xB4, 0x8C ), L"Tan" ),
	SColorAndName( RGB( 0x00, 0x80, 0x80 ), L"Teal" ),
	SColorAndName( RGB( 0xD8, 0xBF, 0xD8 ), L"Thistle" ),
	SColorAndName( RGB( 0xFF, 0x63, 0x47 ), L"Tomato" ),
	SColorAndName( RGB( 0x40, 0xE0, 0xD0 ), L"Turquoise" ),
	SColorAndName( RGB( 0xEE, 0x82, 0xEE ), L"Violet" ),
	SColorAndName( RGB( 0xF5, 0xDE, 0xB3 ), L"Wheat" ),
	SColorAndName( RGB( 0xFF, 0xFF, 0xFF ), L"White" ),
	SColorAndName( RGB( 0xF5, 0xF5, 0xF5 ), L"WhiteSmoke" ),
	SColorAndName( RGB( 0xFF, 0xFF, 0x00 ), L"Yellow" ),
	SColorAndName( RGB( 0x9A, 0xCD, 0x32 ), L"YellowGreen" )
};


CColorPickerCB::CColorPickerCB() : m_hCombo(NULL)
{
}

CColorPickerCB::~CColorPickerCB()
{
}

void CColorPickerCB::Attach(HWND hCombo)
{
	m_hCombo = hCombo;
}

void CColorPickerCB::InitializeDefaultColors(void)
{
	// We Must Be Created First...
	if (!m_hCombo)
		return;

	int iColors = COUNTOF(g_pColors);

	SendMessageW(m_hCombo, CB_RESETCONTENT, 0, 0);	// Clear All Colors

	for (int iColor = 0; iColor < iColors; iColor++)	// For All Colors
	{
		int iAddedItem = (int)SendMessageW(m_hCombo, CB_ADDSTRING, 0, (LPARAM)g_pColors[iColor].m_cColor);
		if (iAddedItem == CB_ERRSPACE)	// If Not Added
		{
			break;
		}
		else	// If Added Successfully
		{
			SendMessageW(m_hCombo, CB_SETITEMDATA, iAddedItem, (LPARAM)g_pColors[iColor].m_crColor);
		}
	}
}

COLORREF CColorPickerCB::GetSelectedColorValue(void)
{
	int iSelectedItem = (int)SendMessageW(m_hCombo, CB_GETCURSEL, 0, 0);	// Get Selected Item

	if (iSelectedItem == CB_ERR)	// If Nothing Selected
	{
		return (RGB(0, 0, 0));		// Return Black
	}
	return (COLORREF)SendMessageW(m_hCombo, CB_GETITEMDATA, iSelectedItem, 0);	// Return Selected Color
}

std::wstring CColorPickerCB::GetSelectedColorName(void)
{
	int iSelectedItem = (int)SendMessageW(m_hCombo, CB_GETCURSEL, 0, 0);

	if (iSelectedItem != CB_ERR)
	{
		wchar_t buf[CCB_MAX_COLOR_NAME];
		SendMessageW(m_hCombo, CB_GETLBTEXT, iSelectedItem, (LPARAM)buf);
		return buf;
	}
	return std::wstring();
}

void CColorPickerCB::SetSelectedColorValue(COLORREF crClr)
{
	int iItems = (int)SendMessageW(m_hCombo, CB_GETCOUNT, 0, 0);

	for (int iItem = 0; iItem < iItems; iItem++)
	{
		if (crClr == (COLORREF)SendMessageW(m_hCombo, CB_GETITEMDATA, iItem, 0))	// If Match Found
		{
			SendMessageW(m_hCombo, CB_SETCURSEL, iItem, 0);	// Select It
			break;
		}
	}
}

void CColorPickerCB::SetSelectedColorName(const std::wstring& sName)
{
	int iItems = (int)SendMessageW(m_hCombo, CB_GETCOUNT, 0, 0);

	for (int iItem = 0; iItem < iItems; iItem++)
	{
		wchar_t cColor[CCB_MAX_COLOR_NAME];
		SendMessageW(m_hCombo, CB_GETLBTEXT, iItem, (LPARAM)cColor);
		if (_wcsicmp(cColor, sName.c_str()) == 0)	// If Match Found
		{
			SendMessageW(m_hCombo, CB_SETCURSEL, iItem, 0);	// Select It
			break;
		}
	}
}

int CColorPickerCB::AddColor(const std::wstring& sColor, COLORREF crColor)
{
	int iIndex = (int)SendMessageW(m_hCombo, CB_ADDSTRING, 0, (LPARAM)sColor.c_str());
	if (iIndex != CB_ERR)	// If Inserted
	{
		SendMessageW(m_hCombo, CB_SETITEMDATA, iIndex, (LPARAM)crColor);	// Set The Color Value
	}
	return (iIndex);	// Return Insertion Location Or Failure Code
}

bool CColorPickerCB::RemoveColor(const std::wstring& sColor)
{
	bool bRemoved = false;
	int iItems = (int)SendMessageW(m_hCombo, CB_GETCOUNT, 0, 0);

	for (int iItem = 0; iItem < iItems; iItem++)
	{
		wchar_t cColor[CCB_MAX_COLOR_NAME];
		SendMessageW(m_hCombo, CB_GETLBTEXT, iItem, (LPARAM)cColor);
		if (_wcsicmp(cColor, sColor.c_str()) == 0)
		{
			if (SendMessageW(m_hCombo, CB_DELETESTRING, iItem, 0) != CB_ERR)
			{
				bRemoved = true;
				break;
			}
		}
	}
	return (bRemoved);
}

bool CColorPickerCB::RemoveColor(COLORREF crClr)
{
	bool bRemoved = false;
	int iItems = (int)SendMessageW(m_hCombo, CB_GETCOUNT, 0, 0);

	for (int iItem = 0; iItem < iItems; iItem++)
	{
		if (crClr == (COLORREF)SendMessageW(m_hCombo, CB_GETITEMDATA, iItem, 0))
		{
			if (SendMessageW(m_hCombo, CB_DELETESTRING, iItem, 0) != CB_ERR)
			{
				bRemoved = true;
				break;
			}
		}
	}
	return (bRemoved);
}

// WM_MEASUREITEM handler for the color picker combo boxes
void CColorPickerCB::MeasureItem(HWND hCombo, LPMEASUREITEMSTRUCT lpMIS)
{
	if (!hCombo)
		return;
	lpMIS->itemHeight = (UINT)SendMessageW(hCombo, CB_GETITEMHEIGHT, 0, 0);
}

// WM_DRAWITEM handler for the color picker combo boxes
void CColorPickerCB::DrawItem(HWND hCombo, LPDRAWITEMSTRUCT pDIStruct)
{
	if (!pDIStruct || pDIStruct->CtlType != ODT_COMBOBOX)
		return;

	COLORREF	crColor = 0;
	COLORREF	crNormal = GetSysColor(COLOR_WINDOW);
	COLORREF	crSelected = GetSysColor(COLOR_HIGHLIGHT);
	COLORREF	crText = GetSysColor(COLOR_WINDOWTEXT);
	HBRUSH		hFrameBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wchar_t		cColor[CCB_MAX_COLOR_NAME] = L"";
	RECT		rItemRect = pDIStruct->rcItem;
	RECT		rBlockRect = rItemRect;
	RECT		rTextRect = rBlockRect;
	HDC			hDC = pDIStruct->hDC;
	int			iFourthWidth = 0;
	int			iItem = pDIStruct->itemID;
	int			iState = pDIStruct->itemState;

	iFourthWidth = (rBlockRect.right - rBlockRect.left) / 4;	// Get 1/4 Of Item Area

	if (iState & ODS_SELECTED)	// If Selected
	{
		SetTextColor(hDC, 0x00FFFFFF & ~(crText));	// Set Inverted Text Color (With Mask)
		SetBkColor(hDC, crSelected);				// Set BG To Highlight Color
		FillRect(hDC, &rBlockRect, (HBRUSH)(crSelected + 1)); // solid fill via brush of color
	}
	else	// If Not Selected
	{
		SetTextColor(hDC, crText);
		SetBkColor(hDC, crNormal);
		FillRect(hDC, &rBlockRect, (HBRUSH)(crNormal + 1));
	}
	if (iState & ODS_FOCUS)
	{
		DrawFocusRect(hDC, &rItemRect);
	}
	//
	//	Calculate Text Area...
	//
	rTextRect.left += (iFourthWidth + 2);
	rTextRect.top += 2;

	//
	//	Calculate Color Block Area..
	//
	InflateRect(&rBlockRect, -2, -2);
	rBlockRect.right = rBlockRect.left + iFourthWidth;

	//
	//	Draw Color Text And Block...
	//
	if (iItem != -1 && hCombo)	// If Not An Empty Item
	{
		int iChars = (int)SendMessageW(hCombo, CB_GETLBTEXT, iItem, (LPARAM)cColor);

		if (iState & ODS_DISABLED)	// If Disabled
		{
			crColor = ::GetSysColor(COLOR_GRAYTEXT);
			SetTextColor(hDC, crColor);
		}
		else
		{
			crColor = (COLORREF)SendMessageW(hCombo, CB_GETITEMDATA, iItem, 0);
		}
		SetBkMode(hDC, TRANSPARENT);
		TextOutW(hDC, rTextRect.left, rTextRect.top, cColor, iChars > 0 ? iChars : 0);

		HBRUSH hColorBrush = CreateSolidBrush(crColor);
		FillRect(hDC, &rBlockRect, hColorBrush);
		DeleteObject(hColorBrush);

		FrameRect(hDC, &rBlockRect, hFrameBrush);
	}
}
