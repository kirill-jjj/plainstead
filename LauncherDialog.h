#pragma once
#include "stdafx.h"

// Launcher dialog (pure Win32)
class LauncherDialog
{
public:
	LauncherDialog(HWND hWndParent = NULL);
	virtual ~LauncherDialog();
	INT_PTR DoModal(HWND hWndParent);
	void EndModal(HWND hWnd, INT_PTR code); // close the modal loop (works with the modeless dialog)
	bool isWantStartGame();
	std::wstring getStartGamePath();
	std::wstring getStartGameTitle();

protected:
	void showInstalledTabControls();
	void showNewTabControls();
	void CreateColumns();
	void RescanInstalled();
	void AddInstalledGame(const std::wstring& name, const std::wstring& version, const std::pair<std::wstring, std::wstring>& path);
	void AddNewGame(const std::wstring& name, const std::wstring& version, const std::wstring& sz, const std::wstring& page, const std::pair<std::wstring, std::wstring>& downloadPageAndInstallName);
	void SetCell(HWND hList, const std::wstring& value, int nRow, int nCol);
	void UpdateNewGamesFromUrl(const std::wstring& url, const std::wstring& temp_xmlfile);
	void ReadNewGamesFromXMLAndAdd(const std::wstring& temp_xmlfile);
	void UpdateNewGamesRssAdditionalInfoFromUrl(const std::wstring& url, const std::wstring& temp_xmlfile);
	void ReadAdditionalInfoFromXMLRss(const std::wstring& temp_xmlfile);
	void ClearNewList();
	void SortColumn(HWND ctrl, int columnIndex, bool ascending);

	std::set<std::wstring> installedGameNameCache; // all names of installed games

	std::vector<std::wstring> repoList;
	std::vector<std::wstring> rssList;
	std::map<std::wstring/*game name*/, std::pair<std::wstring/*approve*/, std::wstring/*info*/> > approveInfo;
	std::map<std::wstring/*game title*/, std::pair<std::wstring/*pubDate*/, std::wstring/*Desc*/> > rssInfo;

	std::wstring m_stGamePath;
	std::wstring m_stGameTitle;
	std::wstring m_gameBaseDir;
	std::wstring currDir;
	bool    m_wantPlay;
	int     m_lastSelFilter;
	bool    m_sortInstalledUp;
	int     m_sortInstalledLastItem;

	bool    m_sortNewUp;
	int     m_sortNewLastItem;

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(HWND hWnd);
	INT_PTR OnCommand(HWND hWnd, int id, int event, HWND hCtl);
	INT_PTR OnNotify(HWND hWnd, NMHDR* pNMHDR);
	BOOL PreTranslateMessage(MSG* pMsg, HWND hWnd);
	void OnOK();
	void OnBnClickedBtnDelGame();
	void OnBnClickedBtnUpdate();
	void OnBnClickedBtnOpenLink();
	void OnBnClickedBtnInstall();
	void OnBnClickedBtnPlayGamem();
	void OnBnClickedBtnResumeoldGame2();
	void OnCbnSelchangeComboFilter();
	void OnHdnItemclickListInstalled(NMHDR* pNMHDR);
	void OnTabSelChange();

	HWND m_hWnd;
	HWND m_hTab;
	HWND m_hListInstalled;
	HWND m_hListNew;
	HWND m_hBtnDelete;
	HWND m_hBtnUpdate;
	HWND m_hBtnInstall;
	HWND m_hBtnOpenLink;
	HWND m_hBtnPlayGame;
	HWND m_hBtnResumeGame;
	HWND m_hComboFiler;
	bool m_running;   // modal loop flag (modeless dialog running as modal)
	INT_PTR m_endCode;
};
