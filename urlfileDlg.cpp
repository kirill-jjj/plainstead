// urlfileDlg.cpp : download dialog implementation (pure Win32)
//

#include "stdafx.h"
#include "resource.h"
#include "urlfileDlg.h"
#include "PlainInstead.h"
#include "unzip.h"

#pragma comment(lib, "urlmon.lib")

#define WM_USER_ENDDOWNLOAD   (WM_USER + 1)
#define WM_USER_DISPLAYSTATUS (WM_USER + 2)

CUrlFileDlg::CUrlFileDlg(const std::wstring& url, const std::wstring& filename)
{
	m_hWnd = NULL;
	m_strURL = url;
	m_selFile = filename;
	goodLoad = false;
	m_hDownloadThread = NULL;
	m_hEventStop = NULL;
	m_hProgress = NULL;
	m_hBytesLoad = NULL;
}

INT_PTR CUrlFileDlg::DoModal(HWND hWndParent)
{
	return DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_URLFILE_DIALOG), hWndParent, DlgProc, (LPARAM)this);
}

INT_PTR CUrlFileDlg::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CUrlFileDlg* pThis = (CUrlFileDlg*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, lParam);
		return ((CUrlFileDlg*)lParam)->OnInitDialog(hWnd);
	case WM_USER_ENDDOWNLOAD:
		if (pThis) pThis->OnEndDownload(wParam);
		return TRUE;
	case WM_USER_DISPLAYSTATUS:
		if (pThis) pThis->OnDisplayStatus(lParam);
		return TRUE;
	case WM_COMMAND:
		if (pThis) return pThis->OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
		break;
	case WM_CLOSE:
		// while downloading, do not close
		if (pThis && pThis->m_hDownloadThread)
			return TRUE;
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

INT_PTR CUrlFileDlg::OnInitDialog(HWND hWnd)
{
	m_hWnd = hWnd;
	m_hProgress = GetDlgItem(hWnd, IDC_PROGRESS);
	m_hBytesLoad = GetDlgItem(hWnd, IDC_EDIT_BYTES_LOAD);
	SendMessageW(m_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessageW(m_hProgress, PBM_SETPOS, 0, 0);
	SetWindowTextW(m_hBytesLoad, L"");
	// start the download
	StartDownload();

	return TRUE;
}

INT_PTR CUrlFileDlg::OnCommand(HWND hWnd, int id, int event, HWND hCtl)
{
	switch (id)
	{
	case IDCANCEL:
		// ask to cancel the download
		if (m_hDownloadThread != NULL)
		{
			int want_exit = MessageBoxW(hWnd, L"Вы действительно хотите прервать загрузку?", L"Загрузка", MB_YESNO | MB_ICONQUESTION);
			if (want_exit == IDYES)
			{
				SetEvent(m_hEventStop); // signal to stop
				// wait for the thread to finish, then close
				WaitForSingleObject(m_hDownloadThread, INFINITE);
				CloseHandle(m_hDownloadThread);
				m_hDownloadThread = NULL;
				EndDialog(hWnd, IDCANCEL);
			}
			return TRUE;
		}
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	case IDOK:
		return TRUE;
	}
	return FALSE;
}

void CUrlFileDlg::StartDownload()
{
	// check if the URL is valid
	if (m_strURL.empty())
	{
		MessageBoxW(m_hWnd, L"Некорректный URL", L"Ошибка", MB_OK | MB_ICONERROR);
		EndDialog(m_hWnd, IDCANCEL);
		return;
	}

	m_hEventStop = CreateEventW(NULL, TRUE, FALSE, NULL); // manual reset, non-signaled

	m_downloadParam.hWnd = m_hWnd;
	m_downloadParam.hEventStop = m_hEventStop;
	m_downloadParam.strURL = m_strURL;
	m_downloadParam.strFileName = m_selFile;

	DWORD dwThreadId = 0;
	m_hDownloadThread = CreateThread(NULL, 0, DownloadThread, &m_downloadParam, 0, &dwThreadId);
	if (m_hDownloadThread == NULL)
	{
		MessageBoxW(m_hWnd, L"Не удалось создать поток загрузки", L"Ошибка", MB_OK | MB_ICONERROR);
		EndDialog(m_hWnd, IDCANCEL);
	}
	ChangeUIDownloading(true);
}

void CUrlFileDlg::ChangeUIDownloading(bool bDownloading)
{
	if (bDownloading)
	{
		SetFocus(m_hProgress);
	}
}

// IBindStatusCallback implementation

CUrlFileDlg::CBSCallbackImpl::CBSCallbackImpl(HWND hWnd, HANDLE hEventStop)
{
	m_hWnd = hWnd;
	m_hEventStop = hEventStop;
	m_ulObjRefCount = 1;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;

	if (::IsEqualIID(riid, __uuidof(IUnknown)))
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}
	else if (::IsEqualIID(riid, __uuidof(IBindStatusCallback)))
	{
		*ppvObject = static_cast<IBindStatusCallback *>(this);
	}

	if (*ppvObject)
	{
		(*reinterpret_cast<LPUNKNOWN *>(ppvObject))->AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CUrlFileDlg::CBSCallbackImpl::AddRef()
{
	return ++m_ulObjRefCount;
}

STDMETHODIMP_(ULONG) CUrlFileDlg::CBSCallbackImpl::Release()
{
	ULONG ret = --m_ulObjRefCount;
	if (ret == 0) delete this;
	return ret;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::OnStartBinding(DWORD, IBinding *)
{
	return S_OK;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::GetPriority(LONG *)
{
	return E_NOTIMPL;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::OnLowResource(DWORD)
{
	return S_OK;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::OnProgress(ULONG ulProgress,
	ULONG ulProgressMax,
	ULONG ulStatusCode,
	LPCWSTR szStatusText)
{
	if (m_hWnd != NULL)
	{
		// inform the dialog to display the current status
		struct DOWNLOADSTATUS
		{
			ULONG ulProgress;
			ULONG ulProgressMax;
			ULONG ulStatusCode;
			LPCWSTR szStatusText;
		};
		DOWNLOADSTATUS downloadStatus = { ulProgress, ulProgressMax, ulStatusCode, szStatusText };
		::SendMessageW(m_hWnd, WM_USER_DISPLAYSTATUS, 0, reinterpret_cast<LPARAM>(&downloadStatus));
	}

	if (m_hEventStop != NULL)
	{
		if (::WaitForSingleObject(m_hEventStop, 0) == WAIT_OBJECT_0)
		{
			::SendMessageW(m_hWnd, WM_USER_DISPLAYSTATUS, 0, NULL);
			return E_ABORT;  // canceled by the user
		}
	}

	return S_OK;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::OnStopBinding(HRESULT, LPCWSTR)
{
	return S_OK;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::GetBindInfo(DWORD *, BINDINFO *)
{
	return S_OK;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::OnDataAvailable(DWORD, DWORD, FORMATETC *, STGMEDIUM *)
{
	return S_OK;
}

STDMETHODIMP CUrlFileDlg::CBSCallbackImpl::OnObjectAvailable(REFIID, IUnknown *)
{
	return S_OK;
}

// the thread procedure: download the file
DWORD CUrlFileDlg::DownloadThread(LPVOID pParam)
{
	DOWNLOADPARAM* pDownloadParam = static_cast<DOWNLOADPARAM*>(pParam);

	CBSCallbackImpl* pbsc = new CBSCallbackImpl(pDownloadParam->hWnd, pDownloadParam->hEventStop);

	HRESULT hr = ::URLDownloadToFileW(NULL,
		pDownloadParam->strURL.c_str(),
		pDownloadParam->strFileName.c_str(),
		0,
		pbsc);

	// report completion; 1 = success, 0 = failure
	::PostMessageW(pDownloadParam->hWnd, WM_USER_ENDDOWNLOAD, SUCCEEDED(hr) ? 1 : 0, 0);

	return 0;
}

// sent when the downloading thread ends; wParam = 1 on success
void CUrlFileDlg::OnEndDownload(WPARAM wParam)
{
	// wait until the thread terminates
	if (m_hDownloadThread)
	{
		WaitForSingleObject(m_hDownloadThread, INFINITE);
		CloseHandle(m_hDownloadThread);
		m_hDownloadThread = NULL;
	}
	if (m_hEventStop)
	{
		CloseHandle(m_hEventStop);
		m_hEventStop = NULL;
	}

	if (!wParam) // failed or canceled
	{
		// delete the partial file
		DeleteFileW(m_selFile.c_str());
	}
	else
	{
		MessageBoxW(m_hWnd, L"Файл загружен!", L"Загрузка", MB_OK | MB_ICONINFORMATION);

		// unzip the game into games\ next to the exe (absolute path:
		// the process current directory is not guaranteed to be the exe dir)
		HZIP hz = OpenZip(m_selFile.c_str(), 0);
		if (hz)
		{
			ZIPENTRY ze;
			GetZipItem(hz, -1, &ze);
			int numitems = ze.index;
			SetUnzipBaseDir(hz, (GetExeDir() + L"games").c_str());
			for (int i = 0; i < numitems; i++)
			{
				GetZipItem(hz, i, &ze);
				UnzipItem(hz, i, ze.name);
			}
			CloseZip(hz);
			// the download is complete
			goodLoad = true;
		}
		else
		{
			MessageBoxW(m_hWnd, L"Скачанный файл не является корректным архивом игры.", L"Ошибка", MB_OK | MB_ICONERROR);
		}
		// delete the archive
		DeleteFileW(m_selFile.c_str());
	}

	EndDialog(m_hWnd, -1);
}

bool CUrlFileDlg::isGoodLoad()
{
	return goodLoad;
}

// display the download progress; lParam = DOWNLOADSTATUS* or NULL if canceled
void CUrlFileDlg::OnDisplayStatus(LPARAM lParam)
{
	struct DOWNLOADSTATUS
	{
		ULONG ulProgress;
		ULONG ulProgressMax;
		ULONG ulStatusCode;
		LPCWSTR szStatusText;
	};
	const DOWNLOADSTATUS* pDownloadStatus = reinterpret_cast<DOWNLOADSTATUS*>(lParam);

	if (pDownloadStatus != NULL)
	{
		std::wstring strStatus = L"  ";
		if (pDownloadStatus->szStatusText)
			strStatus += pDownloadStatus->szStatusText;

		TCHAR strProgress[128];
		swprintf_s(strProgress, L"Скачано байт %lu из %lu",
			pDownloadStatus->ulProgress,
			pDownloadStatus->ulProgressMax);

		int perc = 0;
		if (pDownloadStatus->ulProgressMax > 0)
			perc = (int)(((float)pDownloadStatus->ulProgress / (float)pDownloadStatus->ulProgressMax) * 100.0f);
		SendMessageW(m_hProgress, PBM_SETPOS, perc, 0);
		SetWindowTextW(m_hBytesLoad, strProgress);
	}
}
