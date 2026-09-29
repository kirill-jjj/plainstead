// PlainInsteadView.cpp : implementation of the game view (pure Win32)
//

#include "stdafx.h"
#include "resource.h"
#include "PlainInsteadView.h"
#include "PlainInstead.h"
#include "InterpreterController.h"
#include "MultiSpeech.h"
#include "GlobalManager.h"
#include "IniFile.h"
#include "StdioFileEx.h"
#include <vector>

extern "C" {
#include "instead/instead.h"
}

// message sent by the common Find dialog
UINT WM_FINDREPLACE = ::RegisterWindowMessageW(FINDMSGSTRING);

static std::wstring utf8_to_wide(const char* utf8Str)
{
	if (!utf8Str || !*utf8Str) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wstrTo[0], size_needed);
	wstrTo.resize(wcslen(wstrTo.c_str()));
	return wstrTo;
}

CPlainInsteadView::CPlainInsteadView()
{
	m_hWndMain = NULL;
	m_hOutEdit = NULL;
	m_hListScene = NULL;
	m_hListInv = NULL;
	m_hListWays = NULL;
	m_fontOut = NULL;
	outFontCol = RGB(0, 0, 0);
	outBackCol = RGB(240, 240, 240);
	inFontCol = RGB(0, 0, 0);
	inBackCol = RGB(255, 255, 255);
	currInpHeight = 20;
	useClipboard = false;
	m_auto_say = true;
	m_jump_to_out = false;
	m_hFindDialog = NULL;
	wasFind = false;
	wave_inv = 0;
	wave_ways = 0;
	wave_scene = 0;
	isLogOn = false;
	isStartComment = false;
}

CPlainInsteadView::~CPlainInsteadView()
{
	if (m_fontOut) DeleteObject(m_fontOut);
	if (wave_inv) delete wave_inv;
	if (wave_ways) delete wave_ways;
	if (wave_scene) delete wave_scene;
}

void CPlainInsteadView::CreateView(HWND hWndMain)
{
	// the controller lives in GWLP_USERDATA of the main window:
	// the WndProc fetches it with GetWindowLongPtrW(hWnd, GWLP_USERDATA)
	CPlainInsteadView* view = new CPlainInsteadView();
	view->m_hWndMain = hWndMain;
	SetWindowLongPtrW(hWndMain, GWLP_USERDATA, (LONG_PTR)view);
	// the controls are created directly in the main window; no intermediate container
	view->CreateControls();
}

void CPlainInsteadView::DestroyView()
{
	// detach first, so no message dispatched afterwards sees a stale pointer
	if (m_hWndMain)
		SetWindowLongPtrW(m_hWndMain, GWLP_USERDATA, 0);
	delete this;
}

void CPlainInsteadView::CreateControls()
{
	HINSTANCE hInst = GetModuleHandleW(NULL);
	HWND hWnd = m_hWndMain;
	// multiline output edit
	// multiline output edit (WS_TABSTOP: part of the Tab cycle, see the message loop)
	m_hOutEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
		0, 0, 0, 0, hWnd, (HMENU)(INT_PTR)IDC_EDIT_OUT, hInst, NULL);
	// three lists: scene, inventory, ways
	m_hListScene = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | WS_TABSTOP,
		0, 0, 0, 0, hWnd, (HMENU)(INT_PTR)IDC_LIST_SCENE, hInst, NULL);
	m_hListInv = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | WS_TABSTOP,
		0, 0, 0, 0, hWnd, (HMENU)(INT_PTR)IDC_LIST_INV, hInst, NULL);
	m_hListWays = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | WS_TABSTOP,
		0, 0, 0, 0, hWnd, (HMENU)(INT_PTR)IDC_LIST_WAYS, hInst, NULL);
	// lists handle their own Enter key
	InstallSubclasses();
	OnInitialUpdate();
}

void CPlainInsteadView::OnCtlColor(HWND hWndChild, HDC hDC, UINT nCtlColor)
{
	if (hWndChild == m_hOutEdit)
	{
		SetTextColor(hDC, outFontCol);
		SetBkColor(hDC, outBackCol);
	}
}

void CPlainInsteadView::OnInitialUpdate()
{
	SetWindowTextW(m_hOutEdit,
		L"Управление игрой:\r\n"
		L"Для ввода команд нажмите CTRL+M и выберите игру и нажмите ENTER.\r\n"
		L"Если вы загрузили игру, доступную в библиотеке, то нажмите CTRL+N и загрузите игру из неё.\r\n"
		L"При переходе между комнатами доступен список действий (переходы, предметы и т.д.), выберите нужный и нажмите ENTER.\r\n"
		L"Чтобы применить предмет к объекту, выберите сначала предмет, нажмите ENTER, а затем выберите объект и снова нажмите ENTER.\r\n"
		L"Возвращаться и двигаться по уже выполненным действиям и ответам в истории, можно нажимая на соотв. кнопки.\r\n"
		L"Для повторения, последнего сообщения нажмите соотв. пункт в меню или кнопку.\r\n"
		L"Внимание! Результат игры может не воспроизводиться корректно при прямом запуске exe файла из архива. Рекомендуется распаковывать приложение в отдельную папку с играми."
	);

	// initialize the speech
	MultiSpeech::getInstance();

	// read the settings
	UpdateSettings();

	CIniFile mainSettings;
	useClipboard = mainSettings.GetInt(L"main", L"useClipboard", 0) != 0;

	// initial focus goes to the output edit so screen readers announce the help text
	if (m_hOutEdit && IsWindow(m_hOutEdit))
		SetFocus(m_hOutEdit);
}

void CPlainInsteadView::OnSize(int cx, int cy)
{
	int h_static = 30;
	int sz_list = 200;
	int h_list = (cy / 3 - h_static);
	if (m_hOutEdit) SetWindowPos(m_hOutEdit, NULL, 0, 0, cx - sz_list, cy, SWP_NOACTIVATE | SWP_NOZORDER);
	if (m_hListScene) SetWindowPos(m_hListScene, NULL, 2 + cx - sz_list, h_static, sz_list, h_list, SWP_NOACTIVATE | SWP_NOZORDER);
	if (m_hListInv)   SetWindowPos(m_hListInv, NULL, 2 + cx - sz_list, h_static + h_list + h_static, sz_list, h_list, SWP_NOACTIVATE | SWP_NOZORDER);
	if (m_hListWays)  SetWindowPos(m_hListWays, NULL, 2 + cx - sz_list, h_static + h_list + h_static + h_list + h_static, sz_list, h_list, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CPlainInsteadView::OnMainSetFocus()
{		// focus the output edit, not the empty container
		// (the container has no accessible name and screen readers announce it as "Pane")
		if (m_hOutEdit && IsWindow(m_hOutEdit))
			SetFocus(m_hOutEdit);
}

bool CPlainInsteadView::HandleCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int wmEvent = HIWORD(wParam);
	int id = LOWORD(wParam);

	// control notifications come from the view children
	if (lParam && (m_hOutEdit == (HWND)lParam || m_hListScene == (HWND)lParam ||
		m_hListInv == (HWND)lParam || m_hListWays == (HWND)lParam))
	{
		switch (wmEvent)
		{
		case LBN_SETFOCUS:
			// the exact announcements of the original game view:
			// objects / inventory / ways
			if ((HWND)lParam == m_hListScene) OnLbnSetfocus(m_hListScene, L"объекты");
			else if ((HWND)lParam == m_hListInv) OnLbnSetfocus(m_hListInv, L"инвентарь");
			else if ((HWND)lParam == m_hListWays) OnLbnSetfocus(m_hListWays, L"пути");
			return true;
		case LBN_DBLCLK:
			// double click acts as ENTER
			if ((HWND)lParam == m_hListScene || (HWND)lParam == m_hListInv || (HWND)lParam == m_hListWays)
			{
				// fall through to the keydown logic below
			}
			return true;
		case EN_SETFOCUS:
			if ((HWND)lParam == m_hOutEdit)
			{
				// collapse a full selection made by tabbing into the edit
				DWORD sel = (DWORD)SendMessageW(m_hOutEdit, EM_GETSEL, 0, 0);
				int selFrom = LOWORD(sel);
				int selTo = HIWORD(sel);
				int len = (int)GetWindowTextLengthW(m_hOutEdit);
				if (selFrom == 0 && (len == (selTo - selFrom)))
				{
					SendMessageW(m_hOutEdit, EM_SETSEL, 0, 0);
				}
			}
			return true;
		}
		return false;
	}

	// menu commands handled by the view
	switch (id)
	{
	case ID_FULL_HISTORY: OnFullHistory(); return true;
	case ID_BACK_HIST: OnBackHist(); return true;
	case ID_FORW_HIST: OnForwHist(); return true;
	case ID_MANUAL_STARTER: OnManualStarter(); return true;
	case ID_MANUAL_CMD_LIST: OnManualCmdList(); return true;
	case ID_MANUAL_HOW_PLAY: OnManualHowPlay(); return true;
	case ID_MANUAL_RUK1: OnManualRuk(1); return true;
	case ID_MANUAL_RUK2: OnManualRuk(2); return true;
	case ID_MANUAL_RUK3: OnManualRuk(3); return true;
	case ID_MANUAL_RUK4: OnManualRuk(4); return true;
	case ID_MANUAL_RUK5: OnManualRuk(5); return true;
	case ID_MANUAL_RUK6: OnManualRuk(6); return true;
	case ID_MANUAL_RUK7: OnManualRuk(7); return true;
	case ID_MANUAL_BIG_WRAP: OnManualBigWrap(); return true;
	case ID_FIND_TEXT: OnFindText(); return true;
	case ID_FIND_NEXT: OnFindNext(); return true;
	case ID_HISTORY_STOP: OnHistoryStop(); return true;
	case ID_HISTORY_START: OnHistoryStart(); return true;
	case ID_UPDATE: OnUpdateOutView(); return true;
	case ID_GOTO_SCENE: OnGoto(m_hListScene); return true;
	case ID_GOTO_INV: OnGoto(m_hListInv); return true;
	case ID_GOTO_WAYS: OnGoto(m_hListWays); return true;
	case ID_MENU_LOG: OnMenuLog(); return true;
	case ID_MENU_ADD_COMMENT: OnMenuAddComment(); return true;
	}
	return false;
}

// ---------- PlainInstead Lua bridge ----------
//
// The Lua layer (tiny2/tiny3.lua) collects interactive references as structured
// data: every iface:xref remembers (display name, ref number) in call order and
// returns the clean display text. After each instead_cmd the C++ side reads the
// pairs back with instead.get_refs() and only then clears them with
// instead.clear_refs() (the Lua xref-to-command resolution needs the table
// until the next command runs). No text markers and no regex are involved.
struct BridgeRef
{
	std::wstring name;
	int num;
};

// read (name, number) pairs returned by instead.get_refs(); false if none
static bool FetchRefs(std::vector<BridgeRef>& out)
{
	out.clear();
	if (instead_function("instead.get_refs", NULL))
	{
		instead_clear();
		return false;
	}
	for (int i = 0; ; i++)
	{
		const char* name = instead_retval(i * 2);
		if (!name) break;
		int num = instead_iretval(i * 2 + 1);
		BridgeRef r;
		r.name = utf8_to_wide(name);
		r.num = num;
		out.push_back(r);
	}
	instead_clear();
	return !out.empty();
}

// fill a listbox from the bridge refs; returns the count of items added
static int FillListFromRefs(HWND list, const std::vector<BridgeRef>& refs, std::map<int, int>& map_action)
{
	for (const BridgeRef& r : refs)
	{
		int pos = (int)SendMessageW(list, LB_GETCOUNT, 0, 0);
		SendMessageW(list, LB_INSERTSTRING, pos, (LPARAM)r.name.c_str());
		map_action.insert(std::make_pair(pos, r.num));
	}
	return (int)refs.size();
}

// run one instead command, then collect the refs it produced
static char* InsteadCmdWithRefs(char* cmd, int* rc, std::vector<BridgeRef>& refs)
{
	char* str = instead_cmd(cmd, rc);
	FetchRefs(refs);
	instead_function("instead.clear_refs", NULL);
	instead_clear();
	return str;
}

void CPlainInsteadView::TryInsteadCommand(const std::wstring& textIn, const std::wstring& cmdForLog)
{
	std::wstring resout;
	std::wstring tmp;
	char *p;
	std::map<int, int> prev_map;
	std::wstring userComments;

	// if a comment is being typed, save it into the log
	if (isLogOn && isStartComment) {
		int len = GetWindowTextLengthW(m_hOutEdit);
		std::vector<wchar_t> buf(len + 1);
		GetWindowTextW(m_hOutEdit, &buf[0], len + 1);
		userComments = L"\n*" + std::wstring(&buf[0]);
		isStartComment = false;
		SendMessageW(m_hOutEdit, EM_SETREADONLY, TRUE, 0);
	}

	SendMessageW(m_hListScene, LB_RESETCONTENT, 0, 0);
	prev_map = pos_id_scene;
	pos_id_scene.clear();
	std::vector<BridgeRef> refs;
	if (!textIn.empty())
	{
		bool is_saving = false;
		if (textIn.find(L"save ") != std::wstring::npos)
		{
			GlobalManager::getInstance().userSavedFile();
			is_saving = true;
		}
		std::string command = utf8_encode(textIn);
		// send the command to Instead
		char *str;
		int rc;
		char cmd[256];
		snprintf(cmd, sizeof(cmd), "use %s", command.c_str());
		str = InsteadCmdWithRefs(cmd, &rc, refs);
		if (rc) { /* try go */
			free(str);
			snprintf(cmd, sizeof(cmd), "go %s", command.c_str());
			str = InsteadCmdWithRefs(cmd, &rc, refs);
		}
		if (rc) { /* try act */
			free(str);
			snprintf(cmd, sizeof(cmd), "%s", command.c_str());
			str = InsteadCmdWithRefs(cmd, &rc, refs);
		}
		if (str) {
			resout.append(utf8_to_wide(str));
			resout.append(L"\n");
			if (!is_saving) GlobalManager::getInstance().userNewCommand();
		}
		free(str);
		str = NULL;
	}
	else
	{
		// update the scene
		p = InsteadCmdWithRefs("", NULL, refs);
		if (p && *p) {
			resout.append(utf8_to_wide(p));
			resout.append(L"\n");
		}
	}
	// scene list from the bridge (numbers 0 = status items, beep-only)
	FillListFromRefs(m_hListScene, refs, pos_id_scene);
	if (prev_map.size() != pos_id_scene.size()) {
		wave_scene->play();
	}

	SendMessageW(m_hListWays, LB_RESETCONTENT, 0, 0);
	prev_map = pos_id_ways;
	pos_id_ways.clear();
	p = InsteadCmdWithRefs("way", NULL, refs);
	if (p) free(p);
	FillListFromRefs(m_hListWays, refs, pos_id_ways);
	if (prev_map.size() != pos_id_ways.size()) {
		wave_ways->play();
	}

	p = InsteadCmdWithRefs("inv", NULL, refs);
	if (p) free(p);
	SendMessageW(m_hListInv, LB_RESETCONTENT, 0, 0);
	prev_map = pos_id_inv;
	pos_id_inv.clear();
	FillListFromRefs(m_hListInv, refs, pos_id_inv);
	if (prev_map.size() != pos_id_inv.size()) {
		wave_inv->play();
	}

	// replace \n with \r\n for the edit control
	std::wstring text;
	text.reserve(resout.size() + 16);
	for (size_t i = 0; i < resout.size(); i++)
	{
		if (resout[i] == L'\n' && (i == 0 || resout[i - 1] != L'\r'))
			text += L'\r';
		text += resout[i];
	}
	SetWindowTextW(m_hOutEdit, text.c_str());
	if (!m_jump_to_out) UpdateFocusLogic();
	if (m_auto_say) MultiSpeech::getInstance().Say(resout);
	if (m_jump_to_out) SetFocus(m_hOutEdit);
	if (isLogOn)
	{
		CStdioFileEx flog;
		if (!flog.Open(logFileName.c_str(), OpenFlags::create | OpenFlags::write | OpenFlags::noTruncate))
		{
			MessageBoxW(m_hWndMain, L"Не удалось открыть файл лога! Отключаем логирование.", L"Ошибка", MB_OK | MB_ICONERROR);
			TurnOffLogging();
		}
		flog.SetCodePage(CP_UTF8);
		flog.SeekToEnd();
		// append the user comment, if any
		if (!userComments.empty())
		{
			flog.WriteString(userComments.c_str());
			flog.SeekToEnd();
		}
		std::wstring logLine = L"\n\n>" + cmdForLog + L"\n";
		flog.WriteString(logLine.c_str());
		flog.SeekToEnd();
		flog.WriteString(resout.c_str());
		flog.SeekToEnd();
		int cnt = (int)SendMessageW(m_hListScene, LB_GETCOUNT, 0, 0);
		if (cnt > 0)
		{
			flog.WriteString(L"\nСцена: ");
			flog.SeekToEnd();
			for (int i = 0; i < cnt; i++) {
				int n = (int)SendMessageW(m_hListScene, LB_GETTEXTLEN, i, 0);
				std::vector<wchar_t> itm(n + 1);
				SendMessageW(m_hListScene, LB_GETTEXT, i, (LPARAM)&itm[0]);
				std::wstring s = std::wstring(&itm[0]) + L"; ";
				flog.WriteString(s.c_str());
				flog.SeekToEnd();
			}
		}
		cnt = (int)SendMessageW(m_hListInv, LB_GETCOUNT, 0, 0);
		if (cnt > 0)
		{
			flog.WriteString(L"\nИнвентарь: ");
			flog.SeekToEnd();
			for (int i = 0; i < cnt; i++) {
				int n = (int)SendMessageW(m_hListInv, LB_GETTEXTLEN, i, 0);
				std::vector<wchar_t> itm(n + 1);
				SendMessageW(m_hListInv, LB_GETTEXT, i, (LPARAM)&itm[0]);
				std::wstring s = std::wstring(&itm[0]) + L"; ";
				flog.WriteString(s.c_str());
				flog.SeekToEnd();
			}
		}
		cnt = (int)SendMessageW(m_hListWays, LB_GETCOUNT, 0, 0);
		if (cnt > 0)
		{
			flog.WriteString(L"\nПути: ");
			flog.SeekToEnd();
			for (int i = 0; i < cnt; i++) {
				int n = (int)SendMessageW(m_hListWays, LB_GETTEXTLEN, i, 0);
				std::vector<wchar_t> itm(n + 1);
				SendMessageW(m_hListWays, LB_GETTEXT, i, (LPARAM)&itm[0]);
				std::wstring s = std::wstring(&itm[0]) + L"; ";
				flog.WriteString(s.c_str());
				flog.SeekToEnd();
			}
		}
		flog.Close();
	}
}

void CPlainInsteadView::SetOutputText(const std::wstring& newText, BOOL useHistory)
{
	// detect a switch to the menu mode
	if (GlobalManager::getInstance().isAutoMenuDetect() && !GlobalManager::getInstance().isUseMenu())
	{
		size_t start = 0;
		while (true)
		{
			size_t comma = newText.find(L',', start);
			std::wstring field = newText.substr(start, (comma == std::wstring::npos) ? std::wstring::npos : comma - start);
			if (field == GlobalManager::getInstance().keyMenuString())
			{
				GlobalManager::getInstance().setUseMenu();
				break;
			}
			if (comma == std::wstring::npos) break;
			start = comma + 1;
		}
	}

	if (GlobalManager::getInstance().isUserStartGame() && useHistory)
	{
		// append the response to the history
		GlobalManager::getInstance().appendLastRespond(newText);
	}
	SetWindowTextW(m_hOutEdit, newText.c_str());
	m_newText = newText;
}

// run the command for the currently selected item of a game list
// (restored from the MFC PreTranslateMessage: Enter in scene/inv/ways lists)
bool CPlainInsteadView::OnListEnter()
{
	HWND focused = GetFocus();
	if (focused == m_hListScene)
	{
		int sel_pos = (int)SendMessageW(m_hListScene, LB_GETCURSEL, 0, 0);
		if (sel_pos == LB_ERR) return true;
		if (pos_id_scene.count(sel_pos))
		{
			if (pos_id_scene[sel_pos] == 0)
			{
				MessageBeep(MB_OK);
			}
			else
			{
				int res_pos = pos_id_scene[sel_pos];
				if (res_pos > 1000) res_pos -= 1000;
				std::wstring res = std::to_wstring(res_pos);
				if (!inv_save.empty()) { res = inv_save + L"," + res; inv_save.clear(); }
				int total_list_sz = (int)SendMessageW(m_hListScene, LB_GETCOUNT, 0, 0);
				std::vector<wchar_t> selText(SendMessageW(m_hListScene, LB_GETTEXTLEN, sel_pos, 0) + 1);
				SendMessageW(m_hListScene, LB_GETTEXT, sel_pos, (LPARAM)&selText[0]);
				TryInsteadCommand(res, L"Выбор действия '" + std::wstring(&selText[0]) + L"'");
				if ((int)SendMessageW(m_hListScene, LB_GETCOUNT, 0, 0) == total_list_sz)
					SendMessageW(m_hListScene, LB_SETCURSEL, sel_pos, 0);
				if (SendMessageW(m_hListScene, LB_GETCURSEL, 0, 0) == LB_ERR && SendMessageW(m_hListScene, LB_GETCOUNT, 0, 0) > 0)
					SendMessageW(m_hListScene, LB_SETCURSEL, 0, 0);
			}
			return true;
		}
		return true;
	}
	if (focused == m_hListWays)
	{
		int sel_pos = (int)SendMessageW(m_hListWays, LB_GETCURSEL, 0, 0);
		if (sel_pos == LB_ERR) return true;
		if (pos_id_ways.count(sel_pos))
		{
			if (pos_id_ways[sel_pos] == 0)
			{
				MessageBeep(MB_OK);
			}
			else
			{
				int res_pos = pos_id_ways[sel_pos];
				if (res_pos > 1000) res_pos -= 1000;
				std::wstring res = std::to_wstring(res_pos);
				if (!inv_save.empty()) { res = inv_save + L"," + res; inv_save.clear(); }
				inv_save.clear(); // переход сбрасывает выбор предмета
				int total_list_sz = (int)SendMessageW(m_hListWays, LB_GETCOUNT, 0, 0);
				std::vector<wchar_t> selText(SendMessageW(m_hListWays, LB_GETTEXTLEN, sel_pos, 0) + 1);
				SendMessageW(m_hListWays, LB_GETTEXT, sel_pos, (LPARAM)&selText[0]);
				TryInsteadCommand(res, L"Переход в '" + std::wstring(&selText[0]) + L"'");
				if ((int)SendMessageW(m_hListWays, LB_GETCOUNT, 0, 0) == total_list_sz)
					SendMessageW(m_hListWays, LB_SETCURSEL, sel_pos, 0);
				if (SendMessageW(m_hListWays, LB_GETCURSEL, 0, 0) == LB_ERR && SendMessageW(m_hListWays, LB_GETCOUNT, 0, 0) > 0)
					SendMessageW(m_hListWays, LB_SETCURSEL, 0, 0);
			}
		}
		return true;
	}
	if (focused == m_hListInv)
	{
		int sel_pos = (int)SendMessageW(m_hListInv, LB_GETCURSEL, 0, 0);
		if (sel_pos == LB_ERR) return true;
		if (pos_id_inv.count(sel_pos))
		{
			if (pos_id_inv[sel_pos] == 0)
			{
				MessageBeep(MB_OK);
			}
			else
			{
				bool isMenuItem = (pos_id_inv[sel_pos] > 1000);
				std::wstring res = std::to_wstring(isMenuItem ? pos_id_inv[sel_pos] - 1000 : pos_id_inv[sel_pos]);
				if (inv_save.empty() && !isMenuItem)
				{
					// first stage: take the item
					inv_save = res;
					std::vector<wchar_t> currText(SendMessageW(m_hListInv, LB_GETTEXTLEN, sel_pos, 0) + 1);
					SendMessageW(m_hListInv, LB_GETTEXT, sel_pos, (LPARAM)&currText[0]);
					savedSelInv = &currText[0];
					std::wstring newText = savedSelInv + L" (выбор)";
					SendMessageW(m_hListInv, LB_DELETESTRING, sel_pos, 0);
					SendMessageW(m_hListInv, LB_INSERTSTRING, sel_pos, (LPARAM)newText.c_str());
					SendMessageW(m_hListInv, LB_SETCURSEL, sel_pos, 0);
				}
				else
				{
					// second stage: use the item on the object
					if (res != inv_save && !inv_save.empty()) res = inv_save + L"," + res;
					inv_save.clear();
					int total_list_sz = (int)SendMessageW(m_hListInv, LB_GETCOUNT, 0, 0);
					std::vector<wchar_t> selText(SendMessageW(m_hListInv, LB_GETTEXTLEN, sel_pos, 0) + 1);
					SendMessageW(m_hListInv, LB_GETTEXT, sel_pos, (LPARAM)&selText[0]);
					TryInsteadCommand(res, L"Применяю '" + std::wstring(&selText[0]) + L"' к '" + savedSelInv + L"'");
					if ((int)SendMessageW(m_hListInv, LB_GETCOUNT, 0, 0) == total_list_sz)
						SendMessageW(m_hListInv, LB_SETCURSEL, sel_pos, 0);
					else if (isMenuItem && (int)SendMessageW(m_hListInv, LB_GETCOUNT, 0, 0) > sel_pos)
						SendMessageW(m_hListInv, LB_SETCURSEL, sel_pos, 0);
					if (SendMessageW(m_hListInv, LB_GETCURSEL, 0, 0) == LB_ERR && SendMessageW(m_hListInv, LB_GETCOUNT, 0, 0) > 0)
						SendMessageW(m_hListInv, LB_SETCURSEL, 0, 0);
				}
			}
		}
		return true;
	}
	return false;
}

// WM_FINDREPLACE notification from the common Find dialog
LRESULT CPlainInsteadView::OnFindReplaceMessage(LPARAM lParam)
{
	LPFINDREPLACEW pfr = (LPFINDREPLACEW)lParam;
	if (pfr->Flags & FR_DIALOGTERM)
	{
		m_hFindDialog = NULL;
		return 0;
	}
	if (pfr->Flags & FR_FINDNEXT)
	{
		std::wstring FindName = pfr->lpstrFindWhat ? pfr->lpstrFindWhat : L"";
		bool bMatchCase = (pfr->Flags & FR_MATCHCASE) != 0;
		if (FindStringInEdit(FindName, bMatchCase))
		{
			wasFind = true;
			lastSearchStr = FindName;
			lastMatchCase = bMatchCase;
			// close the dialog after a successful search
			if (m_hFindDialog) { DestroyWindow(m_hFindDialog); m_hFindDialog = NULL; }
		}
		else
		{
			MessageBeep(MB_ICONSTOP);
		}
	}
	return 0;
}

// subclass procedure for the game lists: handles Enter in the list itself
// (canonical SetWindowSubclass instead of an MFC-style PreTranslateMessage)
static LRESULT CALLBACK GameListSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CPlainInsteadView* view = (CPlainInsteadView*)dwRefData;
	if (message == WM_KEYDOWN && wParam == VK_RETURN)
	{
		if (view->OnListEnter())
			return 0;
	}
	return DefSubclassProc(hWnd, message, wParam, lParam);
}

void CPlainInsteadView::InstallSubclasses()
{
	// subclass the game lists so they handle their own keys (Enter),
	// with the view instance passed as reference data
	SetWindowSubclass(m_hListScene, GameListSubclassProc, 1, (DWORD_PTR)this);
	SetWindowSubclass(m_hListInv, GameListSubclassProc, 2, (DWORD_PTR)this);
	SetWindowSubclass(m_hListWays, GameListSubclassProc, 3, (DWORD_PTR)this);
}

void CPlainInsteadView::InitFocusLogic()
{
	SetFocus(m_hListScene);
	UpdateFocusLogic();
}

void CPlainInsteadView::UpdateFocusLogic()
{
	int cntScene = (int)SendMessageW(m_hListScene, LB_GETCOUNT, 0, 0);
	int cntInv = (int)SendMessageW(m_hListInv, LB_GETCOUNT, 0, 0);
	int cntWays = (int)SendMessageW(m_hListWays, LB_GETCOUNT, 0, 0);
	HWND focused = GetFocus();

	if (!IsWindowEnabled(m_hListScene) && cntScene > 0) EnableWindow(m_hListScene, TRUE);
	if (!IsWindowEnabled(m_hListInv) && cntInv > 0) EnableWindow(m_hListInv, TRUE);
	if (!IsWindowEnabled(m_hListWays) && cntWays > 0) EnableWindow(m_hListWays, TRUE);

	if (focused == m_hListScene && cntScene == 0)
	{
		if (cntInv > 0) SetFocus(m_hListInv);
		else if (cntWays > 0) SetFocus(m_hListWays);
		else SetFocus(m_hOutEdit);
	}
	else if (focused == m_hListInv && cntInv == 0)
	{
		if (cntScene > 0) SetFocus(m_hListScene);
		else if (cntWays > 0) SetFocus(m_hListWays);
		else SetFocus(m_hOutEdit);
	}
	else if (focused == m_hListWays && cntWays == 0)
	{
		if (cntScene > 0) SetFocus(m_hListScene);
		else if (cntInv > 0) SetFocus(m_hListInv);
		else SetFocus(m_hOutEdit);
	}

	// like the original game view: empty lists are disabled, so Tab skips them
	// and NVDA announces them as unavailable instead of an empty selection
	if (cntScene == 0) EnableWindow(m_hListScene, FALSE);
	if (cntInv == 0) EnableWindow(m_hListInv, FALSE);
	if (cntWays == 0) EnableWindow(m_hListWays, FALSE);
}

void CPlainInsteadView::UpdateSettings()
{
	CIniFile mainSettings;
	m_auto_say = mainSettings.GetInt(L"main", L"m_CheckAutosay", 1) != 0;
	m_jump_to_out = mainSettings.GetInt(L"main", L"m_CheckSetFocusToOut", 0) != 0;
	UpdateFontSize();
	// announce sounds set
	int currWaveStyle = mainSettings.GetInt(L"main", L"m_ComboStyleAnnounce", 0);
	char wave_pos[30];
	sprintf(wave_pos, "sounds\\scene%d.wav", currWaveStyle + 1);
	if (wave_scene) delete wave_scene;
	wave_scene = new Wave(wave_pos);

	sprintf(wave_pos, "sounds\\inventory%d.wav", currWaveStyle + 1);
	if (wave_inv) delete wave_inv;
	wave_inv = new Wave(wave_pos);

	sprintf(wave_pos, "sounds\\ways%d.wav", currWaveStyle + 1);
	if (wave_ways) delete wave_ways;
	wave_ways = new Wave(wave_pos);
}

void CPlainInsteadView::UpdateFontSize()
{
	CIniFile mainSettings;
	LOGFONTW lf;
	memset(&lf, 0, sizeof(LOGFONTW));
	int currFontHeight = mainSettings.GetInt(L"main", L"fontHeight", 20);
	currInpHeight = currFontHeight + 10;
	lf.lfHeight = currFontHeight;
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Arial");
	if (m_fontOut) DeleteObject(m_fontOut);
	m_fontOut = CreateFontIndirectW(&lf);
	SendMessageW(m_hOutEdit, WM_SETFONT, (WPARAM)m_fontOut, TRUE);

	outFontCol = mainSettings.GetInt(L"main", L"OutFontCol", RGB(0, 0, 0));
	outBackCol = mainSettings.GetInt(L"main", L"OutBackCol", RGB(240, 240, 240));
	inFontCol = mainSettings.GetInt(L"main", L"InFontCol", RGB(0, 0, 0));
	inBackCol = mainSettings.GetInt(L"main", L"InBackCol", RGB(255, 255, 255));
}

static int countExacWnd = 0;
static std::wstring lastWndText;
static std::wstring currWindText;
#define MAX_STABLE_COUNT 3

void CPlainInsteadView::OnFullHistory()
{
	// show the full history, if available
	if (GlobalManager::getInstance().isUserStartGame())
	{
		std::wstring fullHist = GlobalManager::getInstance().fullHistoryData();
		if (!fullHist.empty())
		{
			SetOutputText(fullHist, FALSE);
		}
	}
}

void CPlainInsteadView::OnBackHist()
{
	// step back in the history
	if (GlobalManager::getInstance().isUserStartGame())
	{
		std::wstring prevStep = GlobalManager::getInstance().previosHistoryData();
		if (!prevStep.empty())
		{
			SetOutputText(prevStep, FALSE);
		}
	}
}

void CPlainInsteadView::OnForwHist()
{
	// step forward in the history
	if (GlobalManager::getInstance().isUserStartGame())
	{
		std::wstring nextStep = GlobalManager::getInstance().nextHistoryData();
		if (!nextStep.empty())
		{
			SetOutputText(nextStep, FALSE);
		}
	}
}

void CPlainInsteadView::ShowTextFromResource(LPCWSTR res_id)
{
	bool ok = false;
	HRSRC hResource = FindResourceW(NULL, res_id, L"Text");

	if (hResource)
	{
		HGLOBAL hLoadedResource = LoadResource(NULL, hResource);
		if (hLoadedResource)
		{
			LPVOID pLockedResource = LockResource(hLoadedResource);
			if (pLockedResource)
			{
				DWORD dwResourceSize = SizeofResource(NULL, hResource);
				if (0 != dwResourceSize)
				{
					LPBYTE sData = (LPBYTE)pLockedResource;
					char* text = (char*)sData;
					std::wstring sText = utf8_to_wide(text);
					SetOutputText(sText, FALSE);
					ok = true;
				}
			}
		}
	}

	if (ok == false)
	{
		SetOutputText(L"Ошибка. Не удалось загрузить текст справки.", FALSE);
	}
}

void CPlainInsteadView::OnManualStarter()
{
	ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT1));
}

void CPlainInsteadView::OnManualCmdList()
{
	ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT2));
}

void CPlainInsteadView::OnManualHowPlay()
{
	ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT3));
}

void CPlainInsteadView::OnManualRuk(int num)
{
	switch (num)
	{
	case 1: ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT4)); break;
	case 2: ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT5)); break;
	case 3: ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT6)); break;
	case 4: ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT7)); break;
	case 5: ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT8)); break;
	case 6: ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT9)); break;
	case 7: ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT10)); break;
	}
}

void CPlainInsteadView::OnManualBigWrap()
{
	ShowTextFromResource(MAKEINTRESOURCEW(IDR_TEXT11));
}

void CPlainInsteadView::OnFindText()
{
	if (NULL == m_hFindDialog || !IsWindow(m_hFindDialog))
	{
		// the common find dialog; the buffer must live as long as the dialog
		static wchar_t szFindWhat[256] = L"";
		FINDREPLACEW fr;
		memset(&fr, 0, sizeof(fr));
		fr.lStructSize = sizeof(fr);
		fr.hwndOwner = m_hWndMain;
		fr.lpstrFindWhat = szFindWhat;
		fr.wFindWhatLen = 256;
		fr.Flags = FR_DOWN | FR_HIDEUPDOWN | FR_HIDEWHOLEWORD;
		m_hFindDialog = FindTextW(&fr);
	}
}

bool CPlainInsteadView::FindStringInEdit(std::wstring FindName, bool bMatchCase)
{
	// search in the edit box
	int len = GetWindowTextLengthW(m_hOutEdit);
	std::vector<wchar_t> buf(len + 1);
	GetWindowTextW(m_hOutEdit, &buf[0], len + 1);
	std::wstring sEdit(&buf[0]);
	// convert to lower case if needed
	if (!bMatchCase)
	{
		_wcsupr_s(&FindName[0], FindName.size() + 1);
		FindName = _wcsdup(FindName.c_str());
		std::transform(FindName.begin(), FindName.end(), FindName.begin(), ::towlower);
		std::transform(sEdit.begin(), sEdit.end(), sEdit.begin(), ::towlower);
	}
	// get the current selection
	DWORD sel = (DWORD)SendMessageW(m_hOutEdit, EM_GETSEL, 0, 0);
	int selPos = LOWORD(sel);
	// search
	size_t resPos = sEdit.find(FindName, selPos + 1);
	// found - select
	if (resPos != std::wstring::npos)
	{
		SendMessageW(m_hOutEdit, EM_SETSEL, resPos, resPos + FindName.size());
		SendMessageW(m_hOutEdit, EM_SCROLLCARET, 0, 0);
		return true;
	}
	return false;
}

void CPlainInsteadView::OnFindNext()
{
	if (wasFind)
	{
		if (!FindStringInEdit(lastSearchStr, lastMatchCase)) MessageBeep(MB_ICONSTOP);
	}
}

void CPlainInsteadView::OnHistoryStop()
{
	// disable the history
	GlobalManager::getInstance().enableHistory(false);
	SetOutputText(L"История отключена", FALSE);
}

void CPlainInsteadView::OnHistoryStart()
{
	// enable the history
	GlobalManager::getInstance().enableHistory(true);
	SetOutputText(L"История включена", FALSE);
}


void CPlainInsteadView::OnUpdateOutView()
{
	if (GlobalManager::getInstance().isUserStartGame())
	{
		int sel_pos = 0;
		HWND curr_box = 0;
		HWND focused = GetFocus();
		if (focused == m_hListInv) {
			sel_pos = (int)SendMessageW(m_hListInv, LB_GETCURSEL, 0, 0);
			curr_box = m_hListInv;
		}
		else if (focused == m_hListScene) {
			sel_pos = (int)SendMessageW(m_hListScene, LB_GETCURSEL, 0, 0);
			curr_box = m_hListScene;
		}
		else if (focused == m_hListWays) {
			sel_pos = (int)SendMessageW(m_hListWays, LB_GETCURSEL, 0, 0);
			curr_box = m_hListWays;
		}

		TryInsteadCommand(L"", L"Обновить");

		if (!m_jump_to_out && curr_box)
		{
			int cnt = (int)SendMessageW(curr_box, LB_GETCOUNT, 0, 0);
			if (cnt > sel_pos) {
				SendMessageW(curr_box, LB_SETCURSEL, sel_pos, 0);
			}
		}
	}
}


void CPlainInsteadView::OnLbnSetfocus(HWND hList, const std::wstring& announce)
{
	if (GetFocus() == hList)
	{
		if (hList != m_hListScene) SendMessageW(m_hListScene, LB_SETCURSEL, (WPARAM)-1, 0);
		if (hList != m_hListInv) SendMessageW(m_hListInv, LB_SETCURSEL, (WPARAM)-1, 0);
		if (hList != m_hListWays) SendMessageW(m_hListWays, LB_SETCURSEL, (WPARAM)-1, 0);
		MultiSpeech::getInstance().Say(announce);
	}
	int cnt = (int)SendMessageW(hList, LB_GETCOUNT, 0, 0);
	if ((int)SendMessageW(hList, LB_GETCURSEL, 0, 0) == LB_ERR && cnt > 0)
	{
		SendMessageW(hList, LB_SETCURSEL, 0, 0);
	}
}


void CPlainInsteadView::OnGoto(HWND hList)
{
	int cnt = (int)SendMessageW(hList, LB_GETCOUNT, 0, 0);
	if (cnt > 0 && GetFocus() != hList)
	{
		SetFocus(hList);
	}
}


void CPlainInsteadView::OnMenuLog()
{
	isLogOn = !isLogOn;
	HMENU pMenu = AppGetMainMenu();
	if (pMenu != NULL)
	{
		if (isLogOn) {
			CheckMenuItem(pMenu, ID_MENU_LOG, MF_CHECKED | MF_BYCOMMAND);
			// uses printf() format specifications for time
			SYSTEMTIME st;
			GetLocalTime(&st);
			TCHAR t[32];
			swprintf_s(t, L"%02d%02d%02d_%02d%02d", st.wYear % 100, st.wMonth, st.wDay, st.wHour, st.wMinute);
			std::wstring baseDir = GetExeDir();
			logFileName = baseDir + L"logs\\" + L"log_" + t + L".txt";
			MessageBoxW(m_hWndMain, (std::wstring(L"Включено логирование. Файл сохранится в папке logs под именем: ") + logFileName).c_str(), L"Логирование", MB_OK);
			isStartComment = false;
		}
		else
		{
			CheckMenuItem(pMenu, ID_MENU_LOG, MF_UNCHECKED | MF_BYCOMMAND);
			SendMessageW(m_hOutEdit, EM_SETREADONLY, TRUE, 0);
			isStartComment = false;
		}
	}
}

void CPlainInsteadView::TurnOffLogging()
{
	if (isLogOn) OnMenuLog();
}


void CPlainInsteadView::OnMenuAddComment()
{
	if (isLogOn) {
		isStartComment = true;
		SendMessageW(m_hOutEdit, EM_SETREADONLY, FALSE, 0);
		SetWindowTextW(m_hOutEdit, L"*");
		SetFocus(m_hOutEdit);
		SendMessageW(m_hOutEdit, EM_SETSEL, 1, 1);
	}
	else
	{
		MessageBoxW(m_hWndMain, L"Нужно включить логирование для добавления комментария!", L"Ошибка", MB_OK | MB_ICONERROR);
	}
}
