// LauncherDialog.cpp: game launcher dialog implementation (pure Win32)
//

#include "stdafx.h"
#include "resource.h"
#include "LauncherDialog.h"
#include "urlfileDlg.h"
#include "PlainInstead.h"
#include <wininet.h>
#include <vector>
#include <regex>
#include "StdioFileEx.h"
#include "Markup.h"
#include "IniFile.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "comctl32.lib")

// page ids
#define ID_PAGE_INSTALLED 0 //installed games
#define ID_PAGE_NEW       1 //new (to download)

// keyboard handling for the launcher lists (ENTER/DEL/V/F3),
// implemented as proper subclassing instead of an MFC-style PreTranslateMessage
struct LauncherListData
{
	LauncherDialog* dlg;
};

static LRESULT CALLBACK LauncherListSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LauncherDialog* dlg = (LauncherDialog*)dwRefData;
	if (message == WM_GETDLGCODE)
	{
		// the modal dialog would route ENTER to the default button; let it
		// reach the list instead so the VK_RETURN handler below can act
		MSG* pMsg = (MSG*)lParam;
		if (pMsg && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN
			&& ListView_GetSelectedCount(hWnd) > 0)
			return DLGC_WANTMESSAGE;
	}
	if (message == WM_KEYDOWN)
	{
		int tab = (int)SendMessageW(dlg->m_hTab, TCM_GETCURSEL, 0, 0);
		bool hasSelection = ListView_GetSelectedCount(hWnd) > 0;
		switch (wParam)
		{
		case VK_RETURN:
			if (hasSelection)
			{
				if (hWnd == dlg->m_hListInstalled) dlg->OnBnClickedBtnPlayGamem();
				else if (hWnd == dlg->m_hListNew) dlg->OnBnClickedBtnInstall();
				return 0;
			}
			break;
		case VK_DELETE:
			if (hWnd == dlg->m_hListInstalled && tab == ID_PAGE_INSTALLED && hasSelection)
			{
				dlg->OnBnClickedBtnDelGame();
				return 0;
			}
			break;
		case L'V':
			if (hWnd == dlg->m_hListNew && tab == ID_PAGE_NEW && hasSelection && GetKeyState(VK_CONTROL) >= 0)
			{
				dlg->OnBnClickedBtnOpenLink();
				return 0;
			}
			break;
		case VK_F3:
			if (hWnd == dlg->m_hListInstalled && tab == ID_PAGE_INSTALLED)
			{
				dlg->OnBnClickedBtnResumeoldGame2();
				return 0;
			}
			break;
		case VK_F5:
			if (tab == ID_PAGE_NEW)
			{
				dlg->OnBnClickedBtnUpdate();
				return 0;
			}
			break;
		case L'1':
			if (GetKeyState(VK_CONTROL) < 0 && tab == ID_PAGE_NEW)
			{
				SendMessageW(dlg->m_hTab, TCM_SETCURSEL, ID_PAGE_INSTALLED, 0);
				dlg->showInstalledTabControls();
				return 0;
			}
			break;
		case L'2':
			if (GetKeyState(VK_CONTROL) < 0 && tab == ID_PAGE_INSTALLED)
			{
				SendMessageW(dlg->m_hTab, TCM_SETCURSEL, ID_PAGE_NEW, 0);
				dlg->showNewTabControls();
				return 0;
			}
			break;
		}
	}
	return DefSubclassProc(hWnd, message, wParam, lParam);
}
// filter ids
#define SEL_FILTER_ALL           0 //all games
#define SEL_FILTER_VALID         1 //accessible
#define SEL_FILTER_UNK           2 //unknown

#define N_SUBITEM_LIST_INSTALLED_CAPTION 0 //caption
#define N_SUBITEM_LIST_INSTALLED_ACCESSABLE 1 //accessible
#define N_SUBITEM_LIST_INSTALLED_VERSION    2 //version
#define N_SUBITEM_LIST_INSTALLED_DESC       3 //desc
#define N_SUBITEM_LIST_INSTALLED_DATE       4 //date
#define N_SUBITEM_LIST_INSTALLED_GNAME      5 //internal name

#define N_SUBITEM_LIST_NEW_CAPTION 0 //caption
#define N_SUBITEM_LIST_NEW_ACCESSABLE 1 //accessible
#define N_SUBITEM_LIST_NEW_VERSION    2 //version
#define N_SUBITEM_LIST_NEW_SIZE       3 //size
#define N_SUBITEM_LIST_NEW_DESC       4 //desc
#define N_SUBITEM_LIST_NEW_DATE       5 //publication date
#define N_SUBITEM_LIST_NEW_URL        6 //page URL
#define N_SUBITEM_LIST_NEW_GNAME      7 //internal name
#define N_SUBITEM_LIST_NEW_DWN_URL    8 //URL for downloading
#define N_SUBITEM_LIST_NEW_IS_SANDER  9 //sander flag

LauncherDialog::LauncherDialog(HWND hWndParent)
{
	m_hWnd = NULL;
	m_hTab = NULL;
	m_hListInstalled = NULL;
	m_hListNew = NULL;
	m_hBtnDelete = NULL;
	m_hBtnUpdate = NULL;
	m_hBtnInstall = NULL;
	m_hBtnOpenLink = NULL;
	m_hBtnPlayGame = NULL;
	m_hBtnResumeGame = NULL;
	m_hComboFiler = NULL;
	m_wantPlay = false;
	m_sortInstalledUp = true;
	m_sortNewUp = true;
	m_sortInstalledLastItem = -1;
	m_sortNewLastItem = -1;
}

LauncherDialog::~LauncherDialog()
{
}

INT_PTR LauncherDialog::DoModal(HWND hWndParent)
{
	// canonical modal dialog: Windows runs the message loop and disables the owner
	return DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_LAUNCHERDIALOG), hWndParent, DlgProc, (LPARAM)this);
}

INT_PTR LauncherDialog::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LauncherDialog* pThis = (LauncherDialog*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, lParam);
		return ((LauncherDialog*)lParam)->OnInitDialog(hWnd);

	case WM_SIZE:
		// dialogs are not resizable; ignore
		break;
	case WM_COMMAND:
		if (pThis) return pThis->OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
		break;
	case WM_NOTIFY:
		if (pThis) return pThis->OnNotify(hWnd, (NMHDR*)lParam);
		break;
	case WM_CLOSE:
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

static void ListDirsInDirectory(LPCTSTR dirName, std::vector<std::pair<std::wstring/*full path*/, std::wstring/*name*/> >& filepaths)
{
	filepaths.clear();
	std::wstring wildcard(dirName);
	wildcard += L"\\*.*";
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(wildcard.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (fd.cFileName[0] == L'.')
				continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				filepaths.push_back(std::make_pair(std::wstring(dirName) + L"\\" + fd.cFileName, fd.cFileName));
				continue;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
}

// extract the game name from a LUA file
static std::wstring get_game_name_ru(const std::wstring& inp)
{
	const std::wregex regex_name(L"\\$\\s*Name\\s*\\(\\s*ru\\s*\\)\\s*:?([^\\$]*)\\$");
	std::wsregex_iterator next(inp.begin(), inp.end(), regex_name);
	std::wsregex_iterator end;
	while (next != end) {
		std::wsmatch match = *next;
		if (match.size() == 2)
		{
			std::wstring nameStr = match[1].str();
			nameStr.erase(0, nameStr.find_first_not_of(L" \t"));
			nameStr.erase(nameStr.find_last_not_of(L" \t") + 1);
			return nameStr;
		}
		next++;
	}
	return std::wstring();
}

static std::wstring get_game_name_en(const std::wstring& inp)
{
	const std::wregex regex_name(L"\\$\\s*Name\\s*:?([^\\$]*)\\$");
	std::wsregex_iterator next(inp.begin(), inp.end(), regex_name);
	std::wsregex_iterator end;
	while (next != end) {
		std::wsmatch match = *next;
		if (match.size() == 2)
		{
			std::wstring nameStr = match[1].str();
			nameStr.erase(0, nameStr.find_first_not_of(L" \t"));
			nameStr.erase(nameStr.find_last_not_of(L" \t") + 1);
			return nameStr;
		}
		next++;
	}
	return std::wstring();
}

static std::wstring get_game_version(const std::wstring& inp)
{
	const std::wregex regex_name(L"\\$\\s*Version\\s*:?([^\\$]*)\\$");
	std::wsregex_iterator next(inp.begin(), inp.end(), regex_name);
	std::wsregex_iterator end;
	while (next != end) {
		std::wsmatch match = *next;
		if (match.size() == 2)
		{
			std::wstring nameStr = match[1].str();
			nameStr.erase(0, nameStr.find_first_not_of(L" \t"));
			nameStr.erase(nameStr.find_last_not_of(L" \t") + 1);
			return nameStr;
		}
		next++;
	}
	return std::wstring();
}

INT_PTR LauncherDialog::OnInitDialog(HWND hWnd)
{
	m_hWnd = hWnd;
	m_hTab = GetDlgItem(hWnd, IDC_TAB1);
	m_hListInstalled = GetDlgItem(hWnd, IDC_LIST_INSTALLED);
	m_hListNew = GetDlgItem(hWnd, IDC_LIST_NEW);
	m_hBtnDelete = GetDlgItem(hWnd, IDC_BTN_DEL_GAME);
	m_hBtnUpdate = GetDlgItem(hWnd, IDC_BTN_UPDATE);
	m_hBtnInstall = GetDlgItem(hWnd, IDC_BTN_INSTALL);
	m_hBtnOpenLink = GetDlgItem(hWnd, IDC_BTN_OPEN_LINK);
	m_hBtnPlayGame = GetDlgItem(hWnd, IDC_BTN_PLAY_GAMEM);
	m_hBtnResumeGame = GetDlgItem(hWnd, IDC_BTN_RESUMEOLD_GAME2);
	m_hComboFiler = GetDlgItem(hWnd, IDC_COMBO_FILTER);

	// subclass the game lists so they handle their own keys (ENTER/DEL/V/F3)
	SetWindowSubclass(m_hListInstalled, LauncherListSubclassProc, 1, (DWORD_PTR)this);
	SetWindowSubclass(m_hListNew, LauncherListSubclassProc, 2, (DWORD_PTR)this);

	TCHAR currDirBuf[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, currDirBuf);
	currDir = currDirBuf;
	std::wstring baseDir = GetExeDir();
	SetCurrentDirectoryW(baseDir.c_str());

	m_wantPlay = false;

	// tab pages
	TCITEMW TabItem;
	memset(&TabItem, 0, sizeof(TabItem));
	TabItem.mask = TCIF_TEXT;
	TabItem.pszText = (LPWSTR)L"Установленные игры";
	SendMessageW(m_hTab, TCM_INSERTITEMW, 0, (LPARAM)&TabItem);
	TabItem.pszText = (LPWSTR)L"Загрузить игры";
	SendMessageW(m_hTab, TCM_INSERTITEMW, 1, (LPARAM)&TabItem);
	SendMessageW(m_hComboFiler, CB_ADDSTRING, 0, (LPARAM)L"Все игры");
	SendMessageW(m_hComboFiler, CB_ADDSTRING, 0, (LPARAM)L"Только доступные");
	SendMessageW(m_hComboFiler, CB_ADDSTRING, 0, (LPARAM)L"Только недоступные");

	CIniFile mainSettings;
	m_lastSelFilter = mainSettings.GetInt(L"main", L"mRepoFilter", SEL_FILTER_VALID);
	int cnt = (int)SendMessageW(m_hComboFiler, CB_GETCOUNT, 0, 0);
	if (m_lastSelFilter > cnt - 1) m_lastSelFilter = SEL_FILTER_VALID;
	SendMessageW(m_hComboFiler, CB_SETCURSEL, m_lastSelFilter, 0);

	showInstalledTabControls();

	// restore the built-in game sources if settings.ini has none
	// (first run or after a settings reset; nothing else writes them)
	std::wstring repo1Url;
	mainSettings.GetString(L"Repos", L"repo1", repo1Url, L"");
	if (repo1Url.empty())
	{
		mainSettings.WriteString(L"Repos", L"repo1", L"https://www.instead-games.ru/xml.php");
		mainSettings.WriteString(L"Repos", L"repo2", L"https://www.instead-games.ru/xml2.php");
	}
	std::wstring rss1Url;
	mainSettings.GetString(L"Rss", L"rss1", rss1Url, L"");
	if (rss1Url.empty())
	{
		mainSettings.WriteString(L"Rss", L"rss1", L"https://www.instead-games.ru/rss.php");
		mainSettings.WriteString(L"Rss", L"rss2", L"https://www.instead-games.ru/rss.php?approved=0");
	}

	// set the list control styles
	ListView_SetExtendedListViewStyle(m_hListInstalled, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	ListView_SetExtendedListViewStyle(m_hListNew, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	CreateColumns();

	RescanInstalled();

	// read the RSS sources
	const int TOTAL_AVAIL_RSS = 10;
	for (int i = 0; i < TOTAL_AVAIL_RSS; i++)
	{
		std::wstring currRepo;
		std::wstring keyName;
		keyName = L"rss" + std::to_wstring(i + 1);
		mainSettings.GetString(L"Rss", keyName.c_str(), currRepo, L"");
		if (!currRepo.empty()) {
			rssList.push_back(currRepo);
			std::wstring localName = L"rss" + std::to_wstring(i + 1) + L".xml";
			if (PathFileExistsW(localName.c_str())) ReadAdditionalInfoFromXMLRss(localName);
		}
	}

	// read the game repositories
	const int TOTAL_AVAIL_REPOS = 10;
	for (int i = 0; i < TOTAL_AVAIL_REPOS; i++)
	{
		std::wstring currRepo;
		std::wstring keyName;
		keyName = L"repo" + std::to_wstring(i + 1);
		mainSettings.GetString(L"Repos", keyName.c_str(), currRepo, L"");
		if (!currRepo.empty()) {
			repoList.push_back(currRepo);
			std::wstring localName = L"repo" + std::to_wstring(i + 1) + L".xml";
			if (PathFileExistsW(localName.c_str())) ReadNewGamesFromXMLAndAdd(localName);
		}
	}

	return TRUE;
}

void LauncherDialog::RescanInstalled()
{
	ListView_DeleteAllItems(m_hListInstalled);
	std::wstring baseDir = GetExeDir();
	std::wstring dir = baseDir + L"games";
	m_gameBaseDir = dir;
	std::vector<std::pair<std::wstring, std::wstring> > filePathsAndNames;
	installedGameNameCache.clear();
	ListDirsInDirectory(dir.c_str(), filePathsAndNames);

	for (size_t i = 0; i < filePathsAndNames.size(); i++)
	{
		CStdioFileEx gameFile;
		bool have_file = false;
		if (PathFileExistsW((filePathsAndNames[i].first + L"\\main.lua").c_str()))
		{
			if (gameFile.Open((filePathsAndNames[i].first + L"\\main.lua").c_str(), OpenFlags::read))
				have_file = true;
		}
		else if (PathFileExistsW((filePathsAndNames[i].first + L"\\main3.lua").c_str()))
		{
			if (gameFile.Open((filePathsAndNames[i].first + L"\\main3.lua").c_str(), OpenFlags::read))
				have_file = true;
		}

		if (have_file)
		{
			gameFile.SetCodePage(CP_UTF8);
			std::wstring game_name;
			std::wstring game_name_en;
			std::wstring game_version;
			const int MAX_STR_CNT = 30; //max lines of the file to scan
			int curr_str = 0;
			std::wstring string;
			while (gameFile.ReadString(string))
			{
				if (game_name.empty()) game_name = get_game_name_ru(string);
				if (game_name_en.empty()) game_name_en = get_game_name_en(string);
				if (game_version.empty()) game_version = get_game_version(string);

				if (!game_name.empty() && !game_version.empty())
				{
					AddInstalledGame(game_name, game_version, filePathsAndNames[i]);
					break;
				}
				curr_str++;
				if (curr_str > MAX_STR_CNT) break;
			}

			// one of the fields is missing
			if (game_name.empty() && !game_name_en.empty() && !game_version.empty()) //only the eng. name
			{
				AddInstalledGame(game_name_en, game_version, filePathsAndNames[i]);
			}
			else if (!game_name.empty() && game_version.empty()) //no version
			{
				AddInstalledGame(game_name, L"", filePathsAndNames[i]);
			}
			else if (!game_name_en.empty() && game_version.empty()) //no version, eng. name
			{
				AddInstalledGame(game_name_en, L"", filePathsAndNames[i]);
			}
		}
	}
}

void LauncherDialog::OnTabSelChange()
{
	int nTab = (int)SendMessageW(m_hTab, TCM_GETCURSEL, 0, 0);
	if (nTab == ID_PAGE_INSTALLED) {
		showInstalledTabControls();
	}
	else
	{
		showNewTabControls();
	}
}

void LauncherDialog::showInstalledTabControls()
{
	ShowWindow(m_hListInstalled, SW_SHOW);
	ShowWindow(m_hBtnDelete, SW_SHOW);
	ShowWindow(m_hBtnPlayGame, SW_SHOW);
	ShowWindow(m_hBtnResumeGame, SW_SHOW);

	ShowWindow(m_hListNew, SW_HIDE);
	ShowWindow(m_hBtnUpdate, SW_HIDE);
	ShowWindow(m_hBtnInstall, SW_HIDE);
	ShowWindow(m_hBtnOpenLink, SW_HIDE);
	ShowWindow(m_hComboFiler, SW_HIDE);

	SetFocus(m_hListInstalled);
}

void LauncherDialog::showNewTabControls()
{
	ShowWindow(m_hListInstalled, SW_HIDE);
	ShowWindow(m_hBtnDelete, SW_HIDE);
	ShowWindow(m_hBtnPlayGame, SW_HIDE);
	ShowWindow(m_hBtnResumeGame, SW_HIDE);

	ShowWindow(m_hListNew, SW_SHOW);
	ShowWindow(m_hBtnUpdate, SW_SHOW);
	ShowWindow(m_hBtnInstall, SW_SHOW);
	ShowWindow(m_hBtnOpenLink, SW_SHOW);
	ShowWindow(m_hComboFiler, SW_SHOW);

	SetFocus(m_hListNew);
}

struct PARAMSORT
{
	PARAMSORT(HWND hWnd, int columnIndex, bool ascending)
		:m_hWnd(hWnd)
		, m_ColumnIndex(columnIndex)
		, m_Ascending(ascending)
	{}

	HWND m_hWnd;
	int  m_ColumnIndex;
	bool m_Ascending;
};

// Comparison extracts values from the List-Control
int CALLBACK SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	PARAMSORT& ps = *(PARAMSORT*)lParamSort;

	wchar_t left[256] = L"", right[256] = L"";
	ListView_GetItemText(ps.m_hWnd, (int)lParam1,
		ps.m_ColumnIndex, left, 256);
	ListView_GetItemText(ps.m_hWnd, (int)lParam2,
		ps.m_ColumnIndex, right, 256);

	if (ps.m_Ascending)
		return wcscmp(left, right);
	else
		return wcscmp(right, left);
}

void LauncherDialog::SortColumn(HWND ctrl, int columnIndex, bool ascending)
{
	PARAMSORT paramsort(ctrl, columnIndex, ascending);
	ListView_SortItemsEx(ctrl, SortFunc, (LPARAM)&paramsort);
}

// This function inserts the default values into the listControl
void LauncherDialog::CreateColumns()
{
	LVCOLUMNW list;
	memset(&list, 0, sizeof(list));
	list.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
	list.fmt = LVCFMT_LEFT;
	list.cx = 320;
	list.pszText = (LPWSTR)L"Название";
	list.iSubItem = N_SUBITEM_LIST_INSTALLED_CAPTION;
	ListView_InsertColumn(m_hListInstalled, N_SUBITEM_LIST_INSTALLED_CAPTION, &list);

	list.cx = 100;
	list.pszText = (LPWSTR)L"Доступность";
	list.iSubItem = N_SUBITEM_LIST_INSTALLED_ACCESSABLE;
	ListView_InsertColumn(m_hListInstalled, N_SUBITEM_LIST_INSTALLED_ACCESSABLE, &list);

	list.cx = 100;
	list.pszText = (LPWSTR)L"Версия";
	list.iSubItem = N_SUBITEM_LIST_INSTALLED_VERSION;
	ListView_InsertColumn(m_hListInstalled, N_SUBITEM_LIST_INSTALLED_VERSION, &list);

	list.cx = 200;
	list.pszText = (LPWSTR)L"Описание";
	list.iSubItem = N_SUBITEM_LIST_INSTALLED_DESC;
	ListView_InsertColumn(m_hListInstalled, N_SUBITEM_LIST_INSTALLED_DESC, &list);

	list.cx = 70;
	list.pszText = (LPWSTR)L"Дата";
	list.iSubItem = N_SUBITEM_LIST_INSTALLED_DATE;
	ListView_InsertColumn(m_hListInstalled, N_SUBITEM_LIST_INSTALLED_DATE, &list);

	list.cx = 0;
	list.pszText = (LPWSTR)L"Внутренний имя";
	list.iSubItem = N_SUBITEM_LIST_INSTALLED_GNAME;
	ListView_InsertColumn(m_hListInstalled, N_SUBITEM_LIST_INSTALLED_GNAME, &list);

	/////// new games list
	list.cx = 270;
	list.pszText = (LPWSTR)L"Название";
	list.iSubItem = N_SUBITEM_LIST_NEW_CAPTION;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_CAPTION, &list);

	list.cx = 50;
	list.pszText = (LPWSTR)L"Доступность";
	list.iSubItem = N_SUBITEM_LIST_NEW_ACCESSABLE;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_ACCESSABLE, &list);

	list.cx = 50;
	list.pszText = (LPWSTR)L"Версия";
	list.iSubItem = N_SUBITEM_LIST_NEW_VERSION;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_VERSION, &list);

	list.cx = 50;
	list.pszText = (LPWSTR)L"Размер";
	list.iSubItem = N_SUBITEM_LIST_NEW_SIZE;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_SIZE, &list);

	list.cx = 300;
	list.pszText = (LPWSTR)L"Описание";
	list.iSubItem = N_SUBITEM_LIST_NEW_DESC;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_DESC, &list);

	list.cx = 70;
	list.pszText = (LPWSTR)L"Дата";
	list.iSubItem = N_SUBITEM_LIST_NEW_DATE;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_DATE, &list);

	list.cx = 0;
	list.pszText = (LPWSTR)L"URL";
	list.iSubItem = N_SUBITEM_LIST_NEW_URL;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_URL, &list);

	list.cx = 0;
	list.pszText = (LPWSTR)L"Внутренний имя";
	list.iSubItem = N_SUBITEM_LIST_NEW_GNAME;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_GNAME, &list);

	list.cx = 0;
	list.pszText = (LPWSTR)L"Ссылка на загрузку";
	list.iSubItem = N_SUBITEM_LIST_NEW_DWN_URL;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_DWN_URL, &list);

	list.cx = 0;
	list.pszText = (LPWSTR)L"Ссылка на загрузку";
	list.iSubItem = N_SUBITEM_LIST_NEW_IS_SANDER;
	ListView_InsertColumn(m_hListNew, N_SUBITEM_LIST_NEW_IS_SANDER, &list);
}

void LauncherDialog::AddInstalledGame(const std::wstring& name, const std::wstring& version, const std::pair<std::wstring, std::wstring>& path)
{
	std::wstring mark = L"неизвестно";
	std::wstring info = L"";
	std::wstring date = L"";
	if (approveInfo.count(path.second)) {
		mark = approveInfo[path.second].first;
		info = approveInfo[path.second].second;
	}
	if (rssInfo.count(name)) {
		date = rssInfo[name].first;
		info.append(rssInfo[name].second);
	}

	int cnt = ListView_GetItemCount(m_hListInstalled);
	int col = 0;
	SetCell(m_hListInstalled, name, cnt, col++);
	SetCell(m_hListInstalled, mark, cnt, col++);
	SetCell(m_hListInstalled, version, cnt, col++);
	SetCell(m_hListInstalled, info, cnt, col++);
	SetCell(m_hListInstalled, date, cnt, col++);
	SetCell(m_hListInstalled, path.second, cnt, col++);

	installedGameNameCache.insert(path.second);
}

void LauncherDialog::AddNewGame(const std::wstring& name, const std::wstring& version, const std::wstring& sz, const std::wstring& page, const std::pair<std::wstring, std::wstring>& downloadPageAndInstallName)
{
	std::wstring mark = L"неизвестно";
	std::wstring info = L"";
	std::wstring date = L"";
	if (approveInfo.count(downloadPageAndInstallName.second)) {
		mark = approveInfo[downloadPageAndInstallName.second].first;
		info = approveInfo[downloadPageAndInstallName.second].second;
	}
	if (rssInfo.count(name)) {
		date = rssInfo[name].first;
		info.append(rssInfo[name].second);
	}

	int cnt = ListView_GetItemCount(m_hListNew);
	int col = 0;
	SetCell(m_hListNew, name, cnt, col++);
	SetCell(m_hListNew, mark, cnt, col++);
	SetCell(m_hListNew, version, cnt, col++);
	SetCell(m_hListNew, sz, cnt, col++);
	SetCell(m_hListNew, info, cnt, col++);
	SetCell(m_hListNew, date, cnt, col++);
	SetCell(m_hListNew, page, cnt, col++);
	SetCell(m_hListNew, downloadPageAndInstallName.second, cnt, col++);
	SetCell(m_hListNew, downloadPageAndInstallName.first, cnt, col++);
}

// This function sets the text in the specified SubItem depending on the Row and Column values
void LauncherDialog::SetCell(HWND hList, const std::wstring& value, int nRow, int nCol)
{
	LVITEMW lvItem;
	memset(&lvItem, 0, sizeof(lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nRow;
	lvItem.pszText = (LPWSTR)value.c_str();
	lvItem.iSubItem = nCol;
	if (nCol == 0)
		ListView_InsertItem(hList, &lvItem);
	else
		ListView_SetItem(hList, &lvItem);
}

static int DeleteDirectory(const std::wstring &refcstrRootDirectory,
	bool bDeleteSubdirectories = true)
{
	bool            bSubdirectory = false;
	HANDLE          hFile;
	std::wstring    strFilePath;
	std::wstring    strPattern;
	WIN32_FIND_DATA FileInformation;

	strPattern = refcstrRootDirectory + L"\\*.*";
	hFile = ::FindFirstFileW(strPattern.c_str(), &FileInformation);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FileInformation.cFileName[0] != '.')
			{
				strFilePath = refcstrRootDirectory + L"\\" + FileInformation.cFileName;

				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (bDeleteSubdirectories)
					{
						int iRC = DeleteDirectory(strFilePath, bDeleteSubdirectories);
						if (iRC)
							return iRC;
					}
					else
						bSubdirectory = true;
				}
				else
				{
					if (::SetFileAttributesW(strFilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
						return ::GetLastError();
					if (::DeleteFileW(strFilePath.c_str()) == FALSE)
						return ::GetLastError();
				}
			}
		} while (::FindNextFileW(hFile, &FileInformation) == TRUE);

		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			return dwError;
		else
		{
			if (!bSubdirectory)
			{
				if (::SetFileAttributesW(refcstrRootDirectory.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
					return ::GetLastError();
				if (::RemoveDirectoryW(refcstrRootDirectory.c_str()) == FALSE)
					return ::GetLastError();
			}
		}
	}
	return 0;
}

void LauncherDialog::OnBnClickedBtnDelGame()
{
	int sel = ListView_GetSelectionMark(m_hListInstalled);
	if (sel == -1)
	{
		MessageBoxW(m_hWnd, L"В списке ничего не выбрано!", L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}

	wchar_t gameName[256] = L"";
	ListView_GetItemText(m_hListInstalled, sel, N_SUBITEM_LIST_INSTALLED_GNAME, gameName, 256);
	if (wcscmp(gameName, L"mirror") == 0 || wcscmp(gameName, L"tutorial3") == 0) {
		MessageBoxW(m_hWnd, L"Специальные игры являются частью поставки, а библиотека не даёт их в общий список. Удаление невозможно.", L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}
	wchar_t gameTitle[256] = L"";
	ListView_GetItemText(m_hListInstalled, sel, 0, gameTitle, 256);
	// уточняем удаление игры
	int want_del = MessageBoxW(m_hWnd, (std::wstring(L"Вы действительно хотите удалить игру ") + gameTitle + L"?").c_str(), L"Удаление", MB_YESNOCANCEL | MB_ICONQUESTION);
	if (want_del == IDYES)
	{
		DeleteDirectory(m_gameBaseDir + L"\\" + gameName);
		MessageBoxW(m_hWnd, L"Игра удалена", L"Успех", MB_OK);
		RescanInstalled();
	}
}

void LauncherDialog::ClearNewList()
{
	ListView_DeleteAllItems(m_hListNew);
}

static bool isLocalXml(const std::wstring& path)
{
	const std::wstring filePattern(L"file://");
	return (path.compare(0, filePattern.size(), filePattern) == 0);
}

static std::wstring toLocalFile(const std::wstring& path)
{
	const std::wstring filePattern(L"file://");
	return path.substr(filePattern.size());
}

// download a URL to a file (WinInet, replaces MFC CInternetSession/CHttpFile)
static bool DownloadUrlToFile(const std::wstring& url, const std::wstring& res_path, DWORD* statusCode)
{
	HINTERNET hSession = InternetOpenW(L"PlainInstead", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hSession) return false;
	HINTERNET hUrl = InternetOpenUrlW(hSession, url.c_str(), NULL, 0,
		INTERNET_FLAG_SECURE | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD, 0);
	if (!hUrl)
	{
		InternetCloseHandle(hSession);
		return false;
	}
	// query the HTTP status code
	DWORD status = 0;
	DWORD size = sizeof(status);
	wchar_t codeBuf[16] = L"";
	if (HttpQueryInfoW(hUrl, HTTP_QUERY_STATUS_CODE, codeBuf, &size, NULL))
		status = (DWORD)_wtoi(codeBuf);
	if (statusCode) *statusCode = status;

	bool ok = false;
	FILE* f = _wfopen(res_path.c_str(), L"wb");
	if (f)
	{
		char buf[4096];
		DWORD numread = 0;
		bool readError = false;
		long total = 0;
		while (true)
		{
			if (!InternetReadFile(hUrl, buf, sizeof(buf), &numread))
			{
				readError = true;
				break;
			}
			if (numread == 0) break; // end of the stream
			fwrite(buf, 1, numread, f);
			total += (long)numread;
		}
		fclose(f);
		ok = !readError && total > 0;
	}
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hSession);
	if (!ok)
	{
		// do not leave an empty or truncated cache file behind
		_wremove(res_path.c_str());
	}
	return ok && (status == 0 || status == 200);
}

void LauncherDialog::OnBnClickedBtnUpdate()
{
	approveInfo.clear(); //reset the Approve info
	ClearNewList();
	rssInfo.clear();

	// update the RSS
	for (size_t i = 0; i < rssList.size(); i++)
	{
		if (isLocalXml(rssList[i])) {
			ReadAdditionalInfoFromXMLRss(toLocalFile(rssList[i]));
		}
		else {
			std::wstring localName = L"rss" + std::to_wstring(i + 1) + L".xml";
			UpdateNewGamesRssAdditionalInfoFromUrl(rssList[i], localName);
		}
	}

	// update the repositories
	for (size_t i = 0; i < repoList.size(); i++)
	{
		if (isLocalXml(repoList[i])) {
			ReadNewGamesFromXMLAndAdd(toLocalFile(repoList[i]));
		}
		else {
			std::wstring localName = L"repo" + std::to_wstring(i + 1) + L".xml";
			UpdateNewGamesFromUrl(repoList[i], localName);
		}
	}
}

void LauncherDialog::UpdateNewGamesFromUrl(const std::wstring& url, const std::wstring& temp_xmlfile)
{
	DWORD status = 0;
	if (!DownloadUrlToFile(url, temp_xmlfile, &status))
	{
		MessageBoxW(m_hWnd, L"Не удалось скачать список игр.", L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}
	ReadNewGamesFromXMLAndAdd(temp_xmlfile);
}

void LauncherDialog::UpdateNewGamesRssAdditionalInfoFromUrl(const std::wstring& url, const std::wstring& temp_xmlfile)
{
	DWORD status = 0;
	if (!DownloadUrlToFile(url, temp_xmlfile, &status))
	{
		return;
	}
	ReadAdditionalInfoFromXMLRss(temp_xmlfile);
}

void LauncherDialog::ReadNewGamesFromXMLAndAdd(const std::wstring& temp_xmlfile)
{
	// read the xml into memory
	CStdioFileEx gameFile;
	if (!gameFile.Open(temp_xmlfile.c_str(), OpenFlags::read))
		return;
	gameFile.SetCodePage(CP_UTF8);
	std::wstring xmlDoc;
	std::wstring str;
	while (gameFile.ReadString(str)) xmlDoc.append(str);

	CMarkup xml;
	xml.SetDoc(xmlDoc.c_str());
	while (xml.FindChildElem(L"game"))
	{
		xml.IntoElem();
		xml.FindChildElem(L"name");
		std::wstring csSN = xml.GetChildData();
		xml.FindChildElem(L"title");
		std::wstring csTitle = xml.GetChildData();
		xml.FindChildElem(L"version");
		std::wstring csVersion = xml.GetChildData();
		xml.FindChildElem(L"url");
		std::wstring csDownloadUrl = xml.GetChildData();
		xml.FindChildElem(L"size");
		float csSizeBytes = (float)_wtof(xml.GetChildData().c_str());
		xml.FindChildElem(L"lang");
		std::wstring csLang = xml.GetChildData();
		xml.FindChildElem(L"descurl");
		std::wstring csDescUrl = xml.GetChildData();
		// accessible level, if present in the XML
		if (xml.FindChildElem(L"accessible"))
		{
			std::wstring csAccesible = xml.GetChildData();
			enum AccessibleLevel {
				NotApproved = 1, //not reviewed yet
				Impossible = 2,  //impossible game, requires complex lua
				NoAccesible = 4, //not accessible
				PartAccesible = 8, //partially accessible
				FullAccesible = 16 //fully accessible
			};
			int accesibleLevel = _wtoi(csAccesible.c_str());
			if ((accesibleLevel == PartAccesible) || (accesibleLevel == FullAccesible))
			{
				std::wstring csAccessComment;
				if (xml.FindChildElem(L"accessibleComment"))
				{
					csAccessComment = xml.GetChildData();
				}
				if (!approveInfo.count(csSN))
				{
					approveInfo[csSN] = std::make_pair(L"Да", csAccessComment);
				}
			}
			else if ((accesibleLevel == NoAccesible) || (accesibleLevel == Impossible))
			{
				if (!approveInfo.count(csSN))
				{
					approveInfo[csSN] = std::make_pair(L"Нет", L"");
				}
			}
			else if (accesibleLevel == NotApproved)
			{
				if (!approveInfo.count(csSN))
				{
					approveInfo[csSN] = std::make_pair(L"неизвестно", L"");
				}
			}
		}

		// filter, if the game is not in the list yet
		std::wstring approved_mark;
		if (approveInfo.count(csSN)) approved_mark = approveInfo[csSN].first;
		bool ok_filter = ((m_lastSelFilter == SEL_FILTER_ALL) ||
			((m_lastSelFilter == SEL_FILTER_VALID) && approved_mark == L"Да") ||
			((m_lastSelFilter == SEL_FILTER_UNK) && approved_mark == L"неизвестно")
			);

		// add a downloadable game, if it is not installed yet
		if ((csLang == L"ru") &&
			(installedGameNameCache.count(csSN) == 0) &&
			ok_filter
			)
		{
			std::wstring megabytes_num;
			TCHAR buf[64];
			swprintf_s(buf, L"%.1f МБ. ", csSizeBytes / (1024.0f*1024.0f));
			megabytes_num = buf;
			AddNewGame(csTitle, csVersion, megabytes_num, csDescUrl, std::make_pair(csDownloadUrl, csSN));
		}

		xml.OutOfElem();
	}
}

void LauncherDialog::ReadAdditionalInfoFromXMLRss(const std::wstring& temp_xmlfile)
{
	// read the xml into memory
	CStdioFileEx gameFile;
	if (!gameFile.Open(temp_xmlfile.c_str(), OpenFlags::read))
		return;
	gameFile.SetCodePage(CP_UTF8);
	std::wstring xmlDoc;
	std::wstring str;
	bool firstStr = true;
	while (gameFile.ReadString(str)) {
		//FIXME: due to encoding issues the first line of the RSS xml may lack "<?xml"; restore it
		if (firstStr)
		{
			str = L"<?x" + str;
			firstStr = false;
		}
		xmlDoc.append(str);
	}

	CMarkup xml;
	xml.SetDoc(xmlDoc.c_str());

	if (xml.FindChildElem(L"channel"))
	{
		xml.IntoElem();
		while (xml.FindChildElem(L"item"))
		{
			xml.IntoElem();
			xml.FindChildElem(L"title");
			std::wstring csTitle = xml.GetChildData();
			xml.FindChildElem(L"description");
			std::wstring csDescription = xml.GetChildData();
			xml.FindChildElem(L"pubDate");
			std::wstring csDate = xml.GetChildData();

			// add to the map
			rssInfo[csTitle] = std::make_pair(csDate, csDescription);

			xml.OutOfElem();
		}
		xml.OutOfElem();
	}
}

void LauncherDialog::OnBnClickedBtnOpenLink()
{
	int sel = ListView_GetSelectionMark(m_hListNew);
	if (sel == -1)
	{
		MessageBoxW(m_hWnd, L"В списке ничего не выбрано!", L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}
	wchar_t url_open[2048] = L"";
	ListView_GetItemText(m_hListNew, sel, N_SUBITEM_LIST_NEW_URL, url_open, 2048);
	ShellExecuteW(0, 0, url_open, 0, 0, SW_SHOW);
}

void LauncherDialog::OnBnClickedBtnInstall()
{
	int sel = ListView_GetSelectionMark(m_hListNew);
	if (sel == -1)
	{
		MessageBoxW(m_hWnd, L"В списке ничего не выбрано!", L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}

	wchar_t gameNameBuf[256] = L"";
	ListView_GetItemText(m_hListNew, sel, N_SUBITEM_LIST_NEW_GNAME, gameNameBuf, 256);
	std::wstring gameName = gameNameBuf;
	wchar_t gameTitleBuf[256] = L"";
	ListView_GetItemText(m_hListNew, sel, N_SUBITEM_LIST_NEW_CAPTION, gameTitleBuf, 256);
	std::wstring gameTitle = gameTitleBuf;
	wchar_t gameDwnUrlBuf[2048] = L"";
	ListView_GetItemText(m_hListNew, sel, N_SUBITEM_LIST_NEW_DWN_URL, gameDwnUrlBuf, 2048);
	std::wstring gameDwnUrl = gameDwnUrlBuf;
	//TODO: check for the latest available version

	bool foundInInstalled = false;
	int cnt = ListView_GetItemCount(m_hListInstalled);
	for (int i = 0; i < cnt; i++)
	{
		wchar_t installedGameName[256] = L"";
		ListView_GetItemText(m_hListInstalled, i, N_SUBITEM_LIST_INSTALLED_GNAME, installedGameName, 256);
		if (gameName == installedGameName)
		{
			foundInInstalled = true;
			break;
		}
	}

	if (foundInInstalled)
	{
		int want_run = MessageBoxW(m_hWnd, L"Игра уже установлена. Хотите запустить её?", L"Установлена", MB_YESNOCANCEL | MB_ICONQUESTION);
		if (want_run == IDYES)
		{
			m_wantPlay = true;
			m_stGamePath = m_gameBaseDir + L"\\" + gameName;
			m_stGameTitle = gameTitle;
			EndDialog(m_hWnd, -1);
		}
		return;
	}

	CUrlFileDlg dlg(gameDwnUrl, L"games\\" + gameName + L".zip");
	dlg.DoModal(m_hWnd);
	// after the dialog rescan the installed games
	if (dlg.isGoodLoad())
	{
		RescanInstalled();
	}
}

void LauncherDialog::OnBnClickedBtnPlayGamem()
{
	int sel = ListView_GetSelectionMark(m_hListInstalled);
	if (sel == -1)
	{
		MessageBoxW(m_hWnd, L"В списке ничего не выбрано!", L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}
	wchar_t gameName[256] = L"";
	ListView_GetItemText(m_hListInstalled, sel, N_SUBITEM_LIST_INSTALLED_GNAME, gameName, 256);
	m_wantPlay = true;
	m_stGamePath = m_gameBaseDir + L"\\" + gameName;
	wchar_t gameTitle[256] = L"";
	ListView_GetItemText(m_hListInstalled, sel, N_SUBITEM_LIST_INSTALLED_CAPTION, gameTitle, 256);
	m_stGameTitle = gameTitle;

	EndDialog(m_hWnd, -1);
}

bool LauncherDialog::isWantStartGame()
{
	return m_wantPlay;
}

std::wstring LauncherDialog::getStartGamePath()
{
	return m_stGamePath;
}

std::wstring LauncherDialog::getStartGameTitle()
{
	return m_stGameTitle;
}

void LauncherDialog::OnBnClickedBtnResumeoldGame2()
{
	CIniFile mainSettings;

	m_wantPlay = true;
	std::wstring file, name;
	mainSettings.GetString(L"main", L"lastGameFile", file, L"");
	mainSettings.GetString(L"main", L"lastGameName", name, L"");
	m_stGamePath = file;
	m_stGameTitle = name;

	EndDialog(m_hWnd, -1);
}

void LauncherDialog::OnCbnSelchangeComboFilter()
{
	// the filter changed
	int sel = (int)SendMessageW(m_hComboFiler, CB_GETCURSEL, 0, 0);
	if (m_lastSelFilter != sel)
	{
		CIniFile mainSettings;
		m_lastSelFilter = sel;
		mainSettings.WriteNumber(L"main", L"mRepoFilter", m_lastSelFilter);
		ClearNewList();
		for (size_t i = 0; i < repoList.size(); i++)
		{
			std::wstring localName = L"repo" + std::to_wstring(i + 1) + L".xml";
			ReadNewGamesFromXMLAndAdd(localName);
		}
		for (size_t i = 0; i < rssList.size(); i++)
		{
			std::wstring localName = L"rss" + std::to_wstring(i + 1) + L".xml";
			ReadAdditionalInfoFromXMLRss(localName);
		}
	}
}

void LauncherDialog::OnHdnItemclickListInstalled(NMHDR* pNMHDR)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	int nTab = (int)SendMessageW(m_hTab, TCM_GETCURSEL, 0, 0);

	if (nTab == ID_PAGE_INSTALLED)
	{
		if (m_sortInstalledLastItem != phdr->iItem) {
			m_sortInstalledLastItem = phdr->iItem;
			m_sortInstalledUp = true;
		}
		SortColumn(m_hListInstalled, phdr->iItem, m_sortInstalledUp);
		m_sortInstalledUp = !m_sortInstalledUp;
	}
	else if (nTab == ID_PAGE_NEW)
	{
		if (m_sortNewLastItem != phdr->iItem) {
			m_sortNewLastItem = phdr->iItem;
			m_sortNewUp = true;
		}
		SortColumn(m_hListNew, phdr->iItem, m_sortNewUp);
		m_sortNewUp = !m_sortNewUp;
	}
}

INT_PTR LauncherDialog::OnNotify(HWND hWnd, NMHDR* pNMHDR)
{
	switch (pNMHDR->code)
	{
	case TCN_SELCHANGE:
		if (pNMHDR->hwndFrom == m_hTab)
		{
			OnTabSelChange();
			return TRUE;
		}
		break;
	case HDN_ITEMCLICKW:
	{
		// header clicks arrive from the list view's header control
		HWND hHeader = (HWND)pNMHDR->hwndFrom;
		if (hHeader == ListView_GetHeader(m_hListInstalled) || hHeader == ListView_GetHeader(m_hListNew))
		{
			OnHdnItemclickListInstalled(pNMHDR);
			return TRUE;
		}
		break;
	}
	case NM_DBLCLK:
	{
		// double click acts as ENTER on the installed list
		if (pNMHDR->hwndFrom == m_hListInstalled)
		{
			OnBnClickedBtnPlayGamem();
			return TRUE;
		}
		break;
	}
	}
	return FALSE;
}

INT_PTR LauncherDialog::OnCommand(HWND hWnd, int id, int event, HWND hCtl)
{
	switch (id)
	{
	case IDC_BTN_DEL_GAME: OnBnClickedBtnDelGame(); return TRUE;
	case IDC_BTN_UPDATE: OnBnClickedBtnUpdate(); return TRUE;
	case IDC_BTN_OPEN_LINK: OnBnClickedBtnOpenLink(); return TRUE;
	case IDC_BTN_INSTALL: OnBnClickedBtnInstall(); return TRUE;
	case IDC_BTN_PLAY_GAMEM: OnBnClickedBtnPlayGamem(); return TRUE;
	case IDC_BTN_RESUMEOLD_GAME2: OnBnClickedBtnResumeoldGame2(); return TRUE;
	case IDC_COMBO_FILTER:
		if (event == CBN_SELCHANGE)
		{
			OnCbnSelchangeComboFilter();
		}
		return TRUE;
	case IDCANCEL:
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}
