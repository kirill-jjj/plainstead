// SelectNewGameDialog.h : "select game" dialog, pure Win32
//

#pragma once
#include "stdafx.h"

class CSelectNewGameDialog
{
public:
	CSelectNewGameDialog(std::wstring& selGameFile,
		std::wstring& selName,
		bool& selLastGame);
	INT_PTR DoModal(HWND hWndParent);
	virtual ~CSelectNewGameDialog();

protected:
	std::vector< std::pair<std::wstring/*file name*/, std::wstring/*file description*/> > fileNameDescr;
	std::wstring& m_selGameFile;
	std::wstring& m_selName;
	bool& m_bSelectLastGame;
	std::wstring baseDir;

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(HWND hWnd);
	INT_PTR OnCommand(HWND hWnd, int id, int event, HWND hCtl);
	void OnSize(HWND hWnd, int cx, int cy);
};
