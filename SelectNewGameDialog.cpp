// SelectNewGameDialog.cpp : "select game" dialog implementation (pure Win32)
//

#include "stdafx.h"
#include "resource.h"
#include "SelectNewGameDialog.h"
#include "IniFile.h"

CSelectNewGameDialog::CSelectNewGameDialog(std::wstring& selGameFile, std::wstring& selName, bool& selLastGame)
	: m_selGameFile(selGameFile),
	m_selName(selName),
	m_bSelectLastGame(selLastGame)
{
}

CSelectNewGameDialog::~CSelectNewGameDialog()
{
}

INT_PTR CSelectNewGameDialog::DoModal(HWND hWndParent)
{
	return DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_DIALOG_NEW_GAME), hWndParent, DlgProc, (LPARAM)this);
}

INT_PTR CSelectNewGameDialog::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CSelectNewGameDialog* pThis = (CSelectNewGameDialog*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, lParam);
		return ((CSelectNewGameDialog*)lParam)->OnInitDialog(hWnd);
	case WM_SIZE:
		if (pThis) pThis->OnSize(hWnd, LOWORD(lParam), HIWORD(lParam));
		return TRUE;
	case WM_COMMAND:
		if (pThis) return pThis->OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
		break;
	}
	return FALSE;
}

static void ListDirsInDirectory(LPCTSTR dirName, std::vector<std::wstring>& filepaths)
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
				filepaths.push_back(fd.cFileName);
				continue;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
}

INT_PTR CSelectNewGameDialog::OnInitDialog(HWND hWnd)
{
	std::vector<std::wstring> filePaths;
	HWND hList = GetDlgItem(hWnd, IDC_LIST_GAMES);
	HWND hDescr = GetDlgItem(hWnd, IDC_EDIT_ABOUT);

	TCHAR buff[MAX_PATH];
	memset(buff, 0, sizeof(buff));
	::GetModuleFileNameW(NULL, buff, MAX_PATH);
	baseDir = buff;
	baseDir = baseDir.substr(0, baseDir.find_last_of(L'\\') + 1);
	std::wstring dir = baseDir + L"\\games";
	ListDirsInDirectory(dir.c_str(), filePaths);

	// читаем названия игр из ini
	CIniFile iniFile((baseDir + L"\\games\\info.ini").c_str(), 1024);
	for (size_t i = 0; i < filePaths.size(); i++)
	{
		std::wstring strName;
		iniFile.GetString(L"game_name", filePaths[i].c_str(), strName, L"");
		if (!strName.empty())
		{
			SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)strName.c_str());
			std::wstring strDescr;
			iniFile.GetString(L"game_desc", filePaths[i].c_str(), strDescr, L"Нет описания");
			fileNameDescr.push_back(std::make_pair(filePaths[i], strDescr));
			if (i == 0)
			{
				SetWindowTextW(hDescr, strDescr.c_str());
			}
		}
		else
		{
			SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)filePaths[i].c_str());
			fileNameDescr.push_back(std::make_pair(filePaths[i], L"Нет описания"));
		}
	}

	SendMessageW(hList, LB_SETCURSEL, 0, 0);
	SetFocus(hList);

	// size the dialog to fill the parent area proportionally
	RECT rcParent, rcDlg;
	if (hWnd && GetParent(hWnd) && GetWindowRect(GetParent(hWnd), &rcParent))
	{
		int cx = rcParent.right - rcParent.left;
		int cy = rcParent.bottom - rcParent.top;
		SetWindowPos(hWnd, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
	}
	GetWindowRect(hWnd, &rcDlg);
	OnSize(hWnd, rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top);

	return FALSE;   // we set the focus ourselves
}

void CSelectNewGameDialog::OnSize(HWND hWnd, int cx, int cy)
{
	// пропорциональное растяжение контролов
	const int contHeight = 20;
	HWND hList = GetDlgItem(hWnd, IDC_LIST_GAMES);
	HWND hDescr = GetDlgItem(hWnd, IDC_EDIT_ABOUT);
	if (hList) SetWindowPos(hList, NULL, 0, contHeight, cx, (cy - contHeight * 2) / 2, SWP_NOACTIVATE | SWP_NOZORDER);
	if (hDescr) SetWindowPos(hDescr, NULL, 0, (cy - contHeight * 2) / 2, cx, cy / 2 - contHeight, SWP_NOACTIVATE | SWP_NOZORDER);
}

INT_PTR CSelectNewGameDialog::OnCommand(HWND hWnd, int id, int event, HWND hCtl)
{
	HWND hList = GetDlgItem(hWnd, IDC_LIST_GAMES);
	HWND hDescr = GetDlgItem(hWnd, IDC_EDIT_ABOUT);
	switch (id)
	{
	case IDC_LIST_GAMES:
		if (event == LBN_SELCHANGE)
		{
			int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
			if (sel >= 0 && sel < (int)fileNameDescr.size())
				SetWindowTextW(hDescr, fileNameDescr[sel].second.c_str());
			else
				SetWindowTextW(hDescr, L"");
		}
		else if (event == LBN_DBLCLK)
		{
			// double click = OK
			OnCommand(hWnd, IDOK, 0, hCtl);
		}
		return TRUE;
	case IDOK:
	{
		int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
		if (sel >= 0 && sel < (int)fileNameDescr.size())
		{
			m_selGameFile = baseDir + L"games\\";
			m_selGameFile.append(fileNameDescr[sel].first);
			int n = (int)SendMessageW(hList, LB_GETTEXTLEN, sel, 0);
			std::vector<wchar_t> name(n + 1);
			SendMessageW(hList, LB_GETTEXT, sel, (LPARAM)&name[0]);
			m_selName = &name[0];
			m_bSelectLastGame = false;
		}
		else
		{
			m_selGameFile = L"";
			m_selName = L"";
		}
		EndDialog(hWnd, IDOK);
		return TRUE;
	}
	case IDC_BUTTON_START_LAST_GAME:
	{
		m_bSelectLastGame = true;
		CIniFile mainSettings;
		std::wstring file, name;
		mainSettings.GetString(L"main", L"lastGameFile", file, L"");
		mainSettings.GetString(L"main", L"lastGameName", name, L"");
		m_selGameFile = file;
		m_selName = name;
		EndDialog(hWnd, IDOK);
		return TRUE;
	}
	case IDCANCEL:
		m_selGameFile = L"";
		m_selName = L"";
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}
