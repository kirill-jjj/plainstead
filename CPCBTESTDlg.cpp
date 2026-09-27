// CPCBTESTDlg.cpp : settings dialog implementation (pure Win32)
//

#include "stdafx.h"
#include "resource.h"
#include "ColorPickerCB.h"
#include "CPCBTESTDlg.h"
#include "IniFile.h"
#include "PlainInstead.h"

#define START_FONT 9

CCPCBTESTDlg::CCPCBTESTDlg()
{
	m_hComboOutBack = NULL;
	m_hComboInBack = NULL;
	m_hComboInFont = NULL;
	m_hComboOutFont = NULL;
	m_colorOutBack = RGB(240, 240, 240);
	m_colorInBack = RGB(255, 255, 255);
	m_colorInFont = RGB(0, 0, 0);
	m_colorOutFont = RGB(0, 0, 0);
	m_font = NULL;
	m_hTextExampleOut = NULL;
	m_hTextExampleIn = NULL;
	m_hComboFontSize = NULL;
	m_hCheckAutosay = NULL;
	m_hCheckSetFocusToOut = NULL;
	m_hCheckAutoLog = NULL;
	m_hComboStyleAnnounce = NULL;
	m_currWaveStyle = 0;
	temp_scene_wave = 0;
	temp_inv_wave = 0;
	temp_ways_wave = 0;
	m_valueEdit = 20;
	m_hBrushOut = NULL;
	m_hBrushIn = NULL;
	m_hIcon = NULL;
}

CCPCBTESTDlg::~CCPCBTESTDlg()
{
	if (m_font) DeleteObject(m_font);
	if (m_hBrushOut) DeleteObject(m_hBrushOut);
	if (m_hBrushIn) DeleteObject(m_hBrushIn);
	if (temp_scene_wave) delete temp_scene_wave;
	if (temp_inv_wave) delete temp_inv_wave;
	if (temp_ways_wave) delete temp_ways_wave;
}

INT_PTR CCPCBTESTDlg::DoModal(HWND hWndParent)
{
	return DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_CPCBTEST_DIALOG), hWndParent, DlgProc, (LPARAM)this);
}

INT_PTR CCPCBTESTDlg::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CCPCBTESTDlg* pThis = (CCPCBTESTDlg*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, lParam);
		return ((CCPCBTESTDlg*)lParam)->OnInitDialog(hWnd);
	case WM_COMMAND:
		if (pThis) return pThis->OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
		break;
	case WM_CTLCOLORSTATIC:
		if (pThis && lParam && (HWND)lParam == pThis->m_hTextExampleOut)
		{
			SetTextColor((HDC)wParam, pThis->m_colorOutFont);
			SetBkColor((HDC)wParam, pThis->m_colorOutBack);
			return (INT_PTR)pThis->m_hBrushOut;
		}
		else if (pThis && lParam && (HWND)lParam == pThis->m_hTextExampleIn)
		{
			SetTextColor((HDC)wParam, pThis->m_colorInFont);
			SetBkColor((HDC)wParam, pThis->m_colorInBack);
			return (INT_PTR)pThis->m_hBrushIn;
		}
		break;
	case WM_DRAWITEM:
		if (pThis) return pThis->OnDrawItem(hWnd, (UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
		break;
	}
	return FALSE;
}

INT_PTR CCPCBTESTDlg::OnInitDialog(HWND hWnd)
{
	m_hIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_PLAININSTEAD));
	SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
	SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);

	m_hComboOutBack = GetDlgItem(hWnd, IDC_FOCUSEDCB);
	m_hComboInBack = GetDlgItem(hWnd, IDC_INACTIVECB);
	m_hComboOutFont = GetDlgItem(hWnd, IDC_DISABLEDCB);
	m_hComboInFont = GetDlgItem(hWnd, IDC_DROPPEDCB);
	m_cbOutBack.Attach(m_hComboOutBack);
	m_cbInBack.Attach(m_hComboInBack);
	m_cbOutFont.Attach(m_hComboOutFont);
	m_cbInFont.Attach(m_hComboInFont);
	m_hTextExampleOut = GetDlgItem(hWnd, IDC_STATIC_EXAMPLE_OUT_FONT);
	m_hTextExampleIn = GetDlgItem(hWnd, IDC_STATIC_EXAMPLE_IN_FONT);
	m_hComboFontSize = GetDlgItem(hWnd, IDC_COMBO_FONT_SIZE);
	m_hCheckAutosay = GetDlgItem(hWnd, IDC_CHECK_AUTOSAY);
	m_hCheckSetFocusToOut = GetDlgItem(hWnd, IDC_CHECK_SET_FOCUS_OUT);
	m_hCheckAutoLog = GetDlgItem(hWnd, IDC_CHECK_AUTO_LOG);
	m_hComboStyleAnnounce = GetDlgItem(hWnd, IDC_COMBO_STYLE_ANNOUNCE);

	m_hBrushOut = CreateSolidBrush(m_colorOutBack);
	m_hBrushIn = CreateSolidBrush(m_colorInBack);

	m_cbOutFont.InitializeDefaultColors();
	m_cbInBack.InitializeDefaultColors();
	m_cbOutBack.InitializeDefaultColors();
	m_cbInFont.InitializeDefaultColors();

	CIniFile mainSettings;
	std::wstring fontH;
	mainSettings.GetString(L"main", L"fontHeight", fontH, L"20");

	m_cbOutFont.SetSelectedColorValue(mainSettings.GetInt(L"main", L"OutFontCol", RGB(0, 0, 0)));
	m_cbInBack.SetSelectedColorValue(mainSettings.GetInt(L"main", L"InBackCol", RGB(255, 255, 255)));
	m_cbOutBack.SetSelectedColorValue(mainSettings.GetInt(L"main", L"OutBackCol", RGB(240, 240, 240)));
	m_cbInFont.SetSelectedColorValue(mainSettings.GetInt(L"main", L"InFontCol", RGB(0, 0, 0)));
	m_colorOutFont = m_cbOutFont.GetSelectedColorValue();
	m_colorInBack = m_cbInBack.GetSelectedColorValue();
	m_colorOutBack = m_cbOutBack.GetSelectedColorValue();
	m_colorInFont = m_cbInFont.GetSelectedColorValue();
	CheckDlgButton(hWnd, IDC_CHECK_AUTOSAY, mainSettings.GetInt(L"main", L"m_CheckAutosay", 1) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hWnd, IDC_CHECK_SET_FOCUS_OUT, mainSettings.GetInt(L"main", L"m_CheckSetFocusToOut", 0) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hWnd, IDC_CHECK_AUTO_LOG, mainSettings.GetInt(L"main", L"mCheckAutoLog", 0) ? BST_CHECKED : BST_UNCHECKED);

	m_valueEdit = _wtoi(fontH.c_str());
	UpdateFontSize(hWnd, m_valueEdit);

	// populate the font size combo
	for (int fnt = START_FONT; fnt <= 35; fnt++)
	{
		TCHAR fntText[8];
		swprintf_s(fntText, L"%d", fnt);
		SendMessageW(m_hComboFontSize, CB_INSERTSTRING, fnt - START_FONT, (LPARAM)fntText);
	}
	SendMessageW(m_hComboFontSize, CB_SETCURSEL, m_valueEdit - START_FONT, 0);

	std::wstring baseDir = GetExeDir();
	// populate the announce styles
	for (int style = 1; style <= 5; style++)
	{
		char wave_pos[30];
		sprintf(wave_pos, "sounds\\scene%d.wav", style);
		std::wstring wpos(wave_pos, wave_pos + strlen(wave_pos));
		if (PathFileExistsW((baseDir + L"\\" + wpos).c_str()))
		{
			TCHAR styleText[8];
			swprintf_s(styleText, L"%d", style);
			SendMessageW(m_hComboStyleAnnounce, CB_ADDSTRING, 0, (LPARAM)styleText);
		}
	}
	m_currWaveStyle = mainSettings.GetInt(L"main", L"m_ComboStyleAnnounce", 0);
	SendMessageW(m_hComboStyleAnnounce, CB_SETCURSEL, m_currWaveStyle, 0);
	temp_scene_wave = 0;
	temp_inv_wave = 0;
	temp_ways_wave = 0;
	UpdateTempWave();

	return TRUE;
}

void CCPCBTESTDlg::UpdateTempWave()
{
	char wave_pos[30];

	sprintf(wave_pos, "sounds\\scene%d.wav", m_currWaveStyle + 1);
	if (temp_scene_wave) delete temp_scene_wave;
	temp_scene_wave = new Wave(wave_pos);

	sprintf(wave_pos, "sounds\\inventory%d.wav", m_currWaveStyle + 1);
	if (temp_inv_wave) delete temp_inv_wave;
	temp_inv_wave = new Wave(wave_pos);

	sprintf(wave_pos, "sounds\\ways%d.wav", m_currWaveStyle + 1);
	if (temp_ways_wave) delete temp_ways_wave;
	temp_ways_wave = new Wave(wave_pos);
}

void CCPCBTESTDlg::OnPaint(HWND hWnd)
{
	// icon drawing is handled by the system
}

void CCPCBTESTDlg::ApplySettings(HWND hWnd)
{
	CIniFile mainSettings;
	mainSettings.WriteNumber(L"main", L"fontHeight", m_valueEdit);

	mainSettings.WriteNumber(L"main", L"OutBackCol", (INT)m_cbOutBack.GetSelectedColorValue());
	mainSettings.WriteNumber(L"main", L"InBackCol", (INT)m_cbInBack.GetSelectedColorValue());
	mainSettings.WriteNumber(L"main", L"InFontCol", (INT)m_cbInFont.GetSelectedColorValue());
	mainSettings.WriteNumber(L"main", L"OutFontCol", (INT)m_cbOutFont.GetSelectedColorValue());
	mainSettings.WriteNumber(L"main", L"m_CheckAutosay", IsDlgButtonChecked(hWnd, IDC_CHECK_AUTOSAY) ? 1 : 0);
	mainSettings.WriteNumber(L"main", L"m_CheckSetFocusToOut", IsDlgButtonChecked(hWnd, IDC_CHECK_SET_FOCUS_OUT) ? 1 : 0);
	mainSettings.WriteNumber(L"main", L"mCheckAutoLog", IsDlgButtonChecked(hWnd, IDC_CHECK_AUTO_LOG) ? 1 : 0);
	mainSettings.WriteNumber(L"main", L"m_ComboStyleAnnounce", m_currWaveStyle);
}

INT_PTR CCPCBTESTDlg::OnDrawItem(HWND hWnd, UINT id, LPDRAWITEMSTRUCT lpDIS)
{
	// let the color picker draw its items
	if (lpDIS->CtlType == ODT_COMBOBOX)
	{
		HWND hCombo = (HWND)lpDIS->hwndItem;
		if (hCombo == m_hComboOutBack || hCombo == m_hComboInBack ||
			hCombo == m_hComboOutFont || hCombo == m_hComboInFont)
		{
			CColorPickerCB::DrawItem(hCombo, lpDIS);
			return TRUE;
		}
	}
	return FALSE;
}

INT_PTR CCPCBTESTDlg::OnCommand(HWND hWnd, int id, int event, HWND hCtl)
{
	switch (id)
	{
	case IDOK:
	{
		m_valueEdit = (int)SendMessageW(m_hComboFontSize, CB_GETCURSEL, 0, 0) + START_FONT;
		ApplySettings(hWnd);
		EndDialog(hWnd, IDOK);
		return TRUE;
	}
	case IDCANCEL:
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	case IDC_COMBO_FONT_SIZE:
		if (event == CBN_SELCHANGE)
		{
			int currFont = (int)SendMessageW(m_hComboFontSize, CB_GETCURSEL, 0, 0) + START_FONT;
			UpdateFontSize(hWnd, currFont);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		return TRUE;
	case IDC_COMBO_STYLE_ANNOUNCE:
		if (event == CBN_SELCHANGE)
		{
			int sel = (int)SendMessageW(m_hComboStyleAnnounce, CB_GETCURSEL, 0, 0);
			if (sel != m_currWaveStyle)
			{
				m_currWaveStyle = sel;
				UpdateTempWave();
				// test the announce sounds
				if (temp_scene_wave && temp_inv_wave && temp_ways_wave)
				{
					temp_scene_wave->play(true);
					temp_inv_wave->play(true);
					temp_ways_wave->play(true);
				}
			}
		}
		return TRUE;
	case IDC_BUTTON_CHECK_ANNOUNCE:
		if (temp_scene_wave && temp_inv_wave && temp_ways_wave)
		{
			temp_scene_wave->play(true);
			temp_inv_wave->play(true);
			temp_ways_wave->play(true);
		}
		return TRUE;
	case IDC_FOCUSEDCB:
	case IDC_INACTIVECB:
	case IDC_DISABLEDCB:
	case IDC_DROPPEDCB:
		if (event == CBN_SELCHANGE)
		{
			m_colorOutFont = m_cbOutFont.GetSelectedColorValue();
			m_colorInBack = m_cbInBack.GetSelectedColorValue();
			m_colorOutBack = m_cbOutBack.GetSelectedColorValue();
			m_colorInFont = m_cbInFont.GetSelectedColorValue();
			if (m_hBrushOut) DeleteObject(m_hBrushOut);
			if (m_hBrushIn) DeleteObject(m_hBrushIn);
			m_hBrushOut = CreateSolidBrush(m_colorOutBack);
			m_hBrushIn = CreateSolidBrush(m_colorInBack);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		return TRUE;
	}
	return FALSE;
}

void CCPCBTESTDlg::UpdateFontSize(HWND hWnd, int fontH)
{
	// обновляем размер шрифта
	LOGFONTW lf;
	memset(&lf, 0, sizeof(LOGFONTW));
	lf.lfHeight = fontH;
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Arial");
	if (m_font) DeleteObject(m_font);
	m_font = CreateFontIndirectW(&lf);
	SendMessageW(m_hTextExampleIn, WM_SETFONT, (WPARAM)m_font, TRUE);
	SendMessageW(m_hTextExampleOut, WM_SETFONT, (WPARAM)m_font, TRUE);
}
