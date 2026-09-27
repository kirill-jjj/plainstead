#pragma once
#include "stdafx.h"
#include <urlmon.h>

// Download dialog (pure Win32; replaces the MFC CDialog/CWinThread version)
class CUrlFileDlg
{
public:
	CUrlFileDlg(const std::wstring& url, const std::wstring& filename);
	INT_PTR DoModal(HWND hWndParent);

	void StartDownload();
	bool isGoodLoad();

	// IBindStatusCallback implementation
	class CBSCallbackImpl : public IBindStatusCallback
	{
	public:
		CBSCallbackImpl(HWND hWnd, HANDLE hEventStop);

		// IUnknown methods
		STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();

		// IBindStatusCallback methods
		STDMETHOD(OnStartBinding)(DWORD, IBinding *);
		STDMETHOD(GetPriority)(LONG *);
		STDMETHOD(OnLowResource)(DWORD);
		STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
		STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR);
		STDMETHOD(GetBindInfo)(DWORD *, BINDINFO *);
		STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC *, STGMEDIUM *);
		STDMETHOD(OnObjectAvailable)(REFIID, IUnknown *);

	private:
		ULONG m_ulObjRefCount;
		HWND m_hWnd;
		HANDLE m_hEventStop;
	};

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(HWND hWnd);
	INT_PTR OnCommand(HWND hWnd, int id, int event, HWND hCtl);
	static DWORD WINAPI DownloadThread(LPVOID pParam);

	void ChangeUIDownloading(bool bDownloading = true);
	void OnEndDownload(WPARAM wParam);
	void OnDisplayStatus(LPARAM lParam);

	struct DOWNLOADPARAM
	{
		HWND hWnd;
		HANDLE hEventStop;
		std::wstring strURL;
		std::wstring strFileName;
	};

	HWND m_hWnd;
	std::wstring m_strURL;
	std::wstring m_selFile;
	bool goodLoad;
	HANDLE m_hDownloadThread;
	HANDLE m_hEventStop;
	DOWNLOADPARAM m_downloadParam;
	HWND m_hProgress;
	HWND m_hBytesLoad;
};
