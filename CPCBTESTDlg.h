// CPCBTESTDlg.h : settings dialog, pure Win32
//

#if !defined(CPCBTESTDLG_H__673E7EF4_4211_4178_900D_0D6F7CA9CE94__INCLUDED_)
#define CPCBTESTDLG_H__673E7EF4_4211_4178_900D_0D6F7CA9CE94__INCLUDED_

#include "ColorPickerCB.h"
#include "Wave.h"

class CCPCBTESTDlg
{
public:
	CCPCBTESTDlg();
	INT_PTR DoModal(HWND hWndParent);
	virtual ~CCPCBTESTDlg();

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(HWND hWnd);
	INT_PTR OnCommand(HWND hWnd, int id, int event, HWND hCtl);
	INT_PTR OnDrawItem(HWND hWnd, UINT id, LPDRAWITEMSTRUCT lpDIS);
	void UpdateFontSize(HWND hWnd, int fontH);
	void UpdateTempWave();
	void ApplySettings(HWND hWnd);
	void OnPaint(HWND hWnd);

	HWND m_hComboOutBack;
	HWND m_hComboInBack;
	HWND m_hComboInFont;
	HWND m_hComboOutFont;
	CColorPickerCB m_cbOutBack;
	CColorPickerCB m_cbInBack;
	CColorPickerCB m_cbInFont;
	CColorPickerCB m_cbOutFont;
	COLORREF m_colorOutBack;
	COLORREF m_colorInBack;
	COLORREF m_colorInFont;
	COLORREF m_colorOutFont;
	HFONT m_font;
	HWND m_hTextExampleOut;
	HWND m_hTextExampleIn;
	HWND m_hComboFontSize;
	HWND m_hCheckAutosay;
	HWND m_hCheckSetFocusToOut;
	HWND m_hCheckAutoLog;
	HWND m_hComboStyleAnnounce;
	int m_currWaveStyle;
	Wave* temp_scene_wave;
	Wave* temp_inv_wave;
	Wave* temp_ways_wave;
	int m_valueEdit;
	HBRUSH m_hBrushOut;
	HBRUSH m_hBrushIn;
	HICON m_hIcon;
};

#endif // !defined(CPCBTESTDLG_H__673E7EF4_4211_4178_900D_0D6F7CA9CE94__INCLUDED_)
