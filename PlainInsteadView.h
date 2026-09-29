// PlainInsteadView.h : the game view (edit output + scene/inv/ways listboxes), pure Win32
//

#pragma once
#include "stdafx.h"
#include "Wave.h"

#define ID_TIMER_1 100
#define ID_TIMER_2 101

// message sent by the common Find dialog (registered at startup)
extern UINT WM_FINDREPLACE;

class CPlainInsteadView
{
public:
	CPlainInsteadView();
	~CPlainInsteadView();

	// create the controller and attach it to the main window (WM_CREATE);
	// WndProc reads it back with GetWindowLongPtrW(hWnd, GWLP_USERDATA)
	static void CreateView(HWND hWndMain);
	// detach from the window and destroy the controller (WM_DESTROY)
	void DestroyView();

	HWND GetHwndMain() const { return m_hWndMain; }
	HWND GetHwndOut() const { return m_hOutEdit; }

	// Enter in one of the game lists: run the selected action; called from the list subclass procedure
	bool OnListEnter();
	// install the subclass procedures on the game lists (called once after creation)
	void InstallSubclasses();

	void OnSize(int cx, int cy);
	void OnMainSetFocus();
	// returns TRUE if the WM_COMMAND notification was handled (control notification)
	bool HandleCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
	// WM_FINDREPLACE notification from the common Find dialog (registered message)
	LRESULT OnFindReplaceMessage(LPARAM lParam);
	// colors for the output edit (called from the main WndProc on WM_CTLCOLOREDIT)
	void OnCtlColor(HWND hWndChild, HDC hDC, UINT nCtlColor);

	void SetOutputText(const std::wstring& newText, BOOL useHistory = TRUE);
	void UpdateSettings();
	void InitFocusLogic();
	void UpdateFocusLogic();
	void TryInsteadCommand(const std::wstring& textIn, const std::wstring& cmdForLog = L""); // run a command in the interpreter
	void TurnOffLogging();

private:
	void CreateControls();
	void OnInitialUpdate();
	void ShowTextFromResource(LPCWSTR res_id); // show text from a resource
	void UpdateFontSize();
	void OnFindText();
	void OnFindNext();
	bool FindStringInEdit(std::wstring FindName, bool bMatchCase);
	void OnFullHistory();
	void OnBackHist();
	void OnForwHist();
	void OnManualStarter();
	void OnManualCmdList();
	void OnManualHowPlay();
	void OnManualRuk(int num);
	void OnManualBigWrap();
	void OnHistoryStop();
	void OnHistoryStart();
	void OnUpdateOutView();
	void OnLbnSetfocus(HWND hList, const std::wstring& announce);
	void OnGoto(HWND hList);
	void OnMenuLog();
	void OnMenuAddComment();

	HWND m_hWndMain;    // the top-level frame window (owns the controls directly)
	HWND m_hOutEdit;    // multiline output edit
	HWND m_hListScene;  // scene objects list
	HWND m_hListInv;    // inventory list
	HWND m_hListWays;   // ways list
	HFONT m_fontOut;
	int currInpHeight;
	COLORREF outBackCol;
	COLORREF outFontCol;
	COLORREF inBackCol;
	COLORREF inFontCol;

	std::map<int, int> pos_id_scene;
	std::map<int, int> pos_id_ways;
	std::map<int, int> pos_id_inv;
	std::wstring inv_save; // the first stage of using an item
	std::wstring savedSelInv;

	bool useClipboard;
	bool m_auto_say;
	bool m_jump_to_out;
	bool wasFind;
	std::wstring lastSearchStr;
	bool lastMatchCase;

	std::wstring m_newText;

	// logging
	bool isLogOn;
	bool isStartComment;
	std::wstring logFileName;

	// find dialog state
	HWND m_hFindDialog;

	Wave* wave_inv;
	Wave* wave_scene;
	Wave* wave_ways;
};
