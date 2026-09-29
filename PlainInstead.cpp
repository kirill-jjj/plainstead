// PlainInstead.cpp : main application file (pure Win32, replaces the old MFC CWinApp)
//

#include "stdafx.h"
#include <locale.h>
#include "resource.h"
#include "PlainInsteadView.h"
#include "InterpreterController.h"
#include "GlobalManager.h"
#include "MultiSpeech.h"
#include "SelectNewGameDialog.h"
#include "LauncherDialog.h"
#include "CPCBTESTDlg.h"
#include "IniFile.h"
#include "Tolk.h"
#include "unzip.h"
#pragma comment( lib, "Version.lib" )
#include "bass.h"
#pragma comment( lib, "bass.lib" )
#include "bassmidi.h"
#pragma comment( lib, "bassmidi.lib" )

extern "C" {
#include "instead/instead.h"

extern int instead_sound_init(void);
extern void setGlobalSoundLevel(int volume);
extern int getGlobalSoundLevel();
extern void stopAllSound();
extern int gBassInit;

static int tiny_init(void)
{
	int rc;
	rc = instead_loadfile("tiny.lua");
	if (rc)
		return rc;
	return 0;
}

static struct instead_ext ext;
}

// global application state (was: CPlainInsteadApp members)
static HFONT g_hFontOut = NULL;
static int soundBeforeMute = 80;
static bool isMute = false;
static int effectsBeforeMute = 80;
static bool isMuteEffects = false;
static HWND g_hWndMain = NULL; // the main frame window; the game view controller hangs on its GWLP_USERDATA
static std::wstring currFilePath;
static std::wstring currFileName;
static std::wstring saveDir;
static std::wstring saveGameNameDir;
static HMENU g_hMainMenu = NULL;
static HACCEL g_hAccel = NULL;

HFONT AppGetOutFont() { return g_hFontOut; }
HMENU AppGetMainMenu() { return g_hMainMenu; }
void AppSetOutFont(HFONT hFont) { g_hFontOut = hFont; }

// the game view controller, attached to the main window in WM_CREATE
static CPlainInsteadView* GetView(HWND hWnd)
{
	return (CPlainInsteadView*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
}

// utf8 <-> wide helpers
std::string utf8_encode(const std::wstring &wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::wstring utf8_decode(const char* utf8Str)
{
	if (!utf8Str || !*utf8Str) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wstrTo[0], size_needed);
	wstrTo.resize(wcslen(wstrTo.c_str()));
	return wstrTo;
}

std::wstring GetExeDir()
{
	TCHAR buff[MAX_PATH];
	memset(buff, 0, sizeof(buff));
	::GetModuleFileNameW(NULL, buff, MAX_PATH);
	std::wstring baseDir = buff;
	return baseDir.substr(0, baseDir.find_last_of(L'\\') + 1);
}

// recursively delete a directory (Win32)
static int DeleteDirectory(const std::wstring &refcstrRootDirectory,
	bool              bDeleteSubdirectories = true)
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

// start a game from a file and update the main window
void AppStartNewGameFile(const std::wstring& file, const std::wstring& name)
{
	CIniFile mainSettings;
	int needAutoLog = mainSettings.GetInt(L"main", L"mCheckAutoLog", 0);
	int savedVol = mainSettings.GetInt(L"main", L"mSavedVol", 80);
	soundBeforeMute = savedVol;
	isMute = mainSettings.GetInt(L"main", L"mMuteMusic", 0) != 0;

	int savedEffectsVol = mainSettings.GetInt(L"main", L"mEffectsVol", 80);
	effectsBeforeMute = savedEffectsVol;
	isMuteEffects = mainSettings.GetInt(L"main", L"mMuteEffects", 0) != 0;
	if (!isMuteEffects) Wave::SetVolume(savedEffectsVol);
	else Wave::SetVolume(0);

	stopAllSound();
	if (!isMute) setGlobalSoundLevel(savedVol);
	else setGlobalSoundLevel(0);

	// remember the game for the next launch
	mainSettings.WriteString(L"main", L"lastGameFile", file.c_str());
	mainSettings.WriteString(L"main", L"lastGameName", name.c_str());

	std::wstring baseDir = GetExeDir();
	size_t pos = file.find_last_of(L'\\');
	std::wstring gameName = (pos == std::wstring::npos) ? file : file.substr(pos + 1);
	saveGameNameDir = L"../../saves/" + gameName;
	saveDir = baseDir + L"saves\\" + gameName;
	std::wstring logsDir = baseDir + L"logs\\";
	// create the log directory if it is missing
	if (GetFileAttributesW(logsDir.c_str()) == INVALID_FILE_ATTRIBUTES) {
		SHCreateDirectoryExW(NULL, logsDir.c_str(), NULL);
		if (GetFileAttributesW(logsDir.c_str()) == INVALID_FILE_ATTRIBUTES) {
			MessageBoxW(NULL, L"Не удалось создать директорию для логов!", L"Ошибка", MB_OK | MB_ICONERROR);
		}
	}

	InterpreterController::startGameFile(file, name, needAutoLog);
	GetView(g_hWndMain)->SetOutputText(L"");

	HWND hWndMain = g_hWndMain;
	if (hWndMain)
	{
		SetWindowTextW(hWndMain, name.c_str());
	}

	GlobalManager::getInstance().userStartGame();
	GetView(g_hWndMain)->TryInsteadCommand(L"", L"Запуск игры " + name);
	GetView(g_hWndMain)->InitFocusLogic();
	GlobalManager::lastString = 0;
}

void AppOnFileOpen()
{
	// open a save file
	if (Tolk_IsSpeaking()) Tolk_Silence();
	for (int i = 0; i < 3; i++) //3 попытки загрузки
	{
		wchar_t szFile[MAX_PATH] = L"1.sav";
		OPENFILENAMEW ofn;
		memset(&ofn, 0, sizeof(ofn));
		std::wstring initDir = saveDir;
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = g_hWndMain;
		ofn.lpstrFilter = L"Instead save file (*.sav)\0*.sav\0\0";
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = initDir.c_str();
		ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		int result = GetOpenFileNameW(&ofn);
		if (result)	// если файл выбран
		{
			std::wstring userFilePath = szFile;
			size_t slash = userFilePath.find_last_of(L'\\');
			std::wstring fileDir = (slash == std::wstring::npos) ? L"" : userFilePath.substr(0, slash);
			if (fileDir == saveDir)
			{
				std::wstring userFileName = (slash == std::wstring::npos) ? userFilePath : userFilePath.substr(slash + 1);
				GetView(g_hWndMain)->TryInsteadCommand(L"load " + saveGameNameDir + L"/" + userFileName);
				GetView(g_hWndMain)->TryInsteadCommand(L"", L"Загружено");
				MessageBoxW(NULL, L"Восстановлено!", L"Загрузка", MB_OK);
				return;
			}
			else
			{
				MessageBoxW(NULL, L"Запрещено менять папку. Попробуйте ещё раз.", L"Ошибка", MB_OK | MB_ICONERROR);
			}
		}
		else
		{
			return;
		}
	}
}

void AppOnFileSave()
{
	// save the game
	if (Tolk_IsSpeaking()) Tolk_Silence();
	// создаём папку для сохранения (если её ещё нет)
	if (GetFileAttributes(saveDir.c_str()) == INVALID_FILE_ATTRIBUTES) {
		SHCreateDirectoryExW(NULL, saveDir.c_str(), NULL);
		if (GetFileAttributes(saveDir.c_str()) == INVALID_FILE_ATTRIBUTES) {
			MessageBoxW(NULL, L"Не могу создать директорию для сохранения!", L"Ошибка", MB_OK | MB_ICONERROR);
		}
	}
	for (int i = 0; i < 3; i++) //3 попытки сохранения
	{
		wchar_t szFile[MAX_PATH];
		wcsncpy_s(szFile, (saveDir + L"\\1.sav").c_str(), MAX_PATH - 1);
		szFile[MAX_PATH - 1] = 0;
		OPENFILENAMEW ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = g_hWndMain;
		ofn.lpstrFilter = L"Instead save file (*.sav)\0*.sav\0\0";
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		int result = GetSaveFileNameW(&ofn);
		if (result)	// если файл выбран
		{
			std::wstring userFilePath = szFile;
			size_t slash = userFilePath.find_last_of(L'\\');
			std::wstring fileDir = (slash == std::wstring::npos) ? L"" : userFilePath.substr(0, slash);
			if (fileDir == saveDir)
			{
				std::wstring userFileName = (slash == std::wstring::npos) ? userFilePath : userFilePath.substr(slash + 1);
				GetView(g_hWndMain)->TryInsteadCommand(L"save " + saveGameNameDir + L"/" + userFileName);
				GetView(g_hWndMain)->TryInsteadCommand(L"", L"Сохранение игры");
				MessageBoxW(NULL, L"Сохранено!", L"Сохранение", MB_OK);
				return;
			}
			else
			{
				MessageBoxW(NULL, L"Запрещено менять папку. Попробуйте сохранить ещё раз.", L"Ошибка", MB_OK | MB_ICONERROR);
			}
		}
		else
		{
			return;
		}
	}
}

std::wstring AppGetSaveDir() { return saveDir; }

static void AppNewGameFromFile()
{
	if (!GlobalManager::getInstance().isUserSaveLastFile())
	{
		int res = MessageBoxW(NULL, L"Сохранить текущую игру?", L"Файл не сохранен", MB_YESNOCANCEL);
		if (res == IDYES)
		{
			AppOnFileSave();
		}
		else if (res == IDCANCEL)
		{
			return;
		}
	}

	if (Tolk_IsSpeaking()) Tolk_Silence();
	wchar_t szFile[MAX_PATH] = L"";
	OPENFILENAMEW ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWndMain;
	ofn.lpstrFilter = L"Instead game (*.lua)\0*.lua\0\0";
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	int result = GetOpenFileNameW(&ofn);
	if (result)	// если файл выбран
	{
		currFilePath = szFile;
		size_t slash = currFilePath.find_last_of(L'\\');
		currFileName = (slash == std::wstring::npos) ? currFilePath : currFilePath.substr(slash + 1);
		AppStartNewGameFile(currFilePath, currFileName);
	}
}

void AppOnNewGameFromLib()
{
	if (!GlobalManager::getInstance().isUserSaveLastFile())
	{
		int res = MessageBoxW(NULL, L"Сохранить текущую игру?", L"Файл не сохранен", MB_YESNOCANCEL);
		if (res == IDYES)
		{
			AppOnFileSave();
		}
		else if (res == IDCANCEL)
		{
			return;
		}
	}

	bool useAutosave = false;
	CSelectNewGameDialog selNewGameDialog(currFilePath, currFileName, useAutosave);
	selNewGameDialog.DoModal(g_hWndMain);
	if (!currFilePath.empty())
	{
		AppStartNewGameFile(currFilePath, currFileName);
	}
}

static void AppOnRestartMenu()
{
	if (!GlobalManager::getInstance().isUserSaveLastFile())
	{
		int res = MessageBoxW(NULL, L"Сохранить текущую игру?", L"Файл не сохранен", MB_YESNOCANCEL);
		if (res == IDYES)
		{
			AppOnFileSave();
			AppStartNewGameFile(currFilePath, currFileName);
		}
		else if (res == IDNO)
		{
			AppStartNewGameFile(currFilePath, currFileName);
		}
	}
}

static void AppOnEnterSetup()
{
	CCPCBTESTDlg dlg;
	int nResponse = dlg.DoModal(g_hWndMain);
	if (nResponse == IDOK)
	{
		// перечитываем настройки из ini-файла
		GetView(g_hWndMain)->UpdateSettings();
	}
}

static void AppOnResetAllSettings()
{
	if (MessageBoxW(NULL, L"Вы уверены что хотите сбросить все настройки приложения?", L"Подтверждение сброса настроек", MB_YESNOCANCEL) == IDYES)
	{
		DeleteFileW(L"settings.ini");
		MessageBoxW(NULL, L"Для полного сброса настроек, перезапустите программу", L"Завершение сброса", MB_OK);
	}
}

static void AppQuit()
{
	instead_done();
	if (gBassInit) BASS_Free();
	MultiSpeech::getInstance().Unload();
	PostQuitMessage(0);
}

static void AppOnAppExit()
{
	if (!GlobalManager::getInstance().isUserSaveLastFile())
	{
		int res = MessageBoxW(NULL, L"Файл не сохранен. Всё равно выйти?", L"Файл не сохранен", MB_YESNO);
		if (res == IDYES)
		{
			GlobalManager::getInstance().isIgnoreExitDialog = true;
			HWND hWndMain = g_hWndMain;
			if (hWndMain) DestroyWindow(hWndMain);
		}
	}
	else
	{
		HWND hWndMain = g_hWndMain;
		if (hWndMain) DestroyWindow(hWndMain);
	}
}

static void AppOnVolumeDown()
{
	if (getGlobalSoundLevel() > 0) {
		int newLevel = getGlobalSoundLevel() - 10;
		soundBeforeMute = newLevel;
		CIniFile mainSettings;
		mainSettings.WriteNumber(L"main", L"mSavedVol", newLevel);
		setGlobalSoundLevel(newLevel);
	}
}

static void AppOnVolumeUp()
{
	if (getGlobalSoundLevel() < 100) {
		int newLevel = getGlobalSoundLevel() + 10;
		soundBeforeMute = newLevel;
		CIniFile mainSettings;
		mainSettings.WriteNumber(L"main", L"mSavedVol", newLevel);
		setGlobalSoundLevel(newLevel);
	}
}

static void AppOnVolumeOn()
{
	HMENU pMenu = g_hMainMenu;
	if (pMenu != NULL)
	{
		CIniFile mainSettings;
		if (isMute) {
			setGlobalSoundLevel(soundBeforeMute);
			isMute = false;
			mainSettings.WriteNumber(L"main", L"mMuteMusic", 0);
			CheckMenuItem(pMenu, ID_VOLUME_ON, MF_CHECKED | MF_BYCOMMAND);
		}
		else
		{
			setGlobalSoundLevel(0);
			isMute = true;
			mainSettings.WriteNumber(L"main", L"mMuteMusic", 1);
			CheckMenuItem(pMenu, ID_VOLUME_ON, MF_UNCHECKED | MF_BYCOMMAND);
		}
	}
}

static void AppOnListsndOn()
{
	HMENU pMenu = g_hMainMenu;
	if (pMenu != NULL)
	{
		CIniFile mainSettings;
		if (isMuteEffects) {
			Wave::SetVolume(effectsBeforeMute);
			isMuteEffects = false;
			mainSettings.WriteNumber(L"main", L"mMuteEffects", 0);
			CheckMenuItem(pMenu, ID_LISTSND_ON, MF_CHECKED | MF_BYCOMMAND);
		}
		else
		{
			Wave::SetVolume(0);
			isMuteEffects = true;
			mainSettings.WriteNumber(L"main", L"mMuteEffects", 1);
			CheckMenuItem(pMenu, ID_LISTSND_ON, MF_UNCHECKED | MF_BYCOMMAND);
		}
	}
}

static void AppOnListsndDown()
{
	if (Wave::GetVolume() > 0) {
		int newLevel = Wave::GetVolume() - 10;
		effectsBeforeMute = newLevel;
		CIniFile mainSettings;
		mainSettings.WriteNumber(L"main", L"mEffectsVol", newLevel);
		Wave::SetVolume(newLevel);
	}
}

static void AppOnListsndUp()
{
	if (Wave::GetVolume() < 100) {
		int newLevel = Wave::GetVolume() + 10;
		effectsBeforeMute = newLevel;
		CIniFile mainSettings;
		mainSettings.WriteNumber(L"main", L"mEffectsVol", newLevel);
		Wave::SetVolume(newLevel);
	}
}

// add a zip game to the library
static void AppOnAddGameToLib()
{
	if (Tolk_IsSpeaking()) Tolk_Silence();
	wchar_t szFile[MAX_PATH] = L"";
	OPENFILENAMEW ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWndMain;
	ofn.lpstrFilter = L"Архив с игрой (*.zip)\0*.zip\0\0";
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	int result = GetOpenFileNameW(&ofn);
	if (result)	// если файл выбран
	{
		std::wstring baseDir = GetExeDir();
		SetCurrentDirectoryW(baseDir.c_str());

		// распаковка через zip utils
		HZIP hz = OpenZip(szFile, 0);
		ZIPENTRY ze;
		GetZipItem(hz, -1, &ze);
		int numitems = ze.index;
		SetUnzipBaseDir(hz, (baseDir + L"games").c_str());
		bool have_main_lua = false;
		std::wstring game_name;
		for (int i = 0; i < numitems; i++)
		{
			GetZipItem(hz, i, &ze);
			if (i == 0) game_name = ze.name; //первый файл-папка, со слешем на конце
			if (ze.name == game_name + L"main.lua") {
				have_main_lua = true;
				break;
			}
			else if (ze.name == game_name + L"main3.lua") {
				have_main_lua = true;
				break;
			}
		}
		// убираем слеш с конца
		if (!game_name.empty() && (game_name[game_name.length() - 1] == L'/' || game_name[game_name.length() - 1] == L'\\'))
			game_name = game_name.substr(0, game_name.length() - 1);
		// проверяем, это ли игра инстеда
		if (!have_main_lua)
		{
			CloseZip(hz);
			MessageBoxW(NULL, L"Архив не является игрой INSTEAD.", L"Ошибка", MB_OK | MB_ICONERROR);
			return;
		}
		// проверяем, есть ли такая папка уже
		if (GetFileAttributesW((L"games\\" + game_name).c_str()) != INVALID_FILE_ATTRIBUTES) {
			int res = MessageBoxW(NULL, L"Данная игра уже есть в библиотеке. Вы хотите её заменить?", L"Замена", MB_YESNO);
			if (res == IDYES)
			{
				DeleteDirectory(L"games\\" + game_name);
			}
			else
			{
				CloseZip(hz);
				return;
			}
		}
		// начинаем распаковку
		GetZipItem(hz, -1, &ze);
		numitems = ze.index;
		for (int i = 0; i < numitems; i++)
		{
			GetZipItem(hz, i, &ze);
			UnzipItem(hz, i, ze.name);
		}

		CloseZip(hz);
		MessageBoxW(NULL, L"Игра установлена в библиотеку", L"Успех", MB_OK);
	}
}

static void AppOnOpenManager()
{
	LauncherDialog dlg;
	dlg.DoModal(g_hWndMain);
	if (dlg.isWantStartGame())
	{
		currFilePath = dlg.getStartGamePath();
		currFileName = dlg.getStartGameTitle();
		AppStartNewGameFile(currFilePath, currFileName);
	}
}

// About dialog
class CAboutDlg
{
public:
	CAboutDlg(const std::wstring& dictors) : m_textVoice(dictors) {}

	INT_PTR DoModal(HWND hWndParent)
	{
		return DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_ABOUTBOX), hWndParent, AboutProc, (LPARAM)this);
	}

private:
	static INT_PTR CALLBACK AboutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			CAboutDlg* pThis = (CAboutDlg*)lParam;
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
			SetDlgItemTextW(hWnd, IDC_STATIC_VOICE_HELP, pThis->m_textVoice.c_str());
			// product version (first two numbers)
			std::wstring ver = L"?.?.?";
			TCHAR szFilename[MAX_PATH + 1] = { 0 };
			if (GetModuleFileNameW(NULL, szFilename, MAX_PATH))
			{
				DWORD dwHandle = 0;
				DWORD dwSize = GetFileVersionInfoSizeW(szFilename, &dwHandle);
				if (dwSize)
				{
					std::vector<BYTE> pbVersionInfo(dwSize);
					VS_FIXEDFILEINFO* pFileInfo = NULL;
					UINT puLenFileInfo = 0;
					if (GetFileVersionInfoW(szFilename, 0, dwSize, &pbVersionInfo[0]) &&
						VerQueryValueW(&pbVersionInfo[0], L"\\", (LPVOID*)&pFileInfo, &puLenFileInfo) && pFileInfo)
					{
						TCHAR buf[32];
						swprintf_s(buf, L"%d.%d",
							(pFileInfo->dwProductVersionMS >> 16) & 0xffff,
							(pFileInfo->dwProductVersionMS >> 0) & 0xffff);
						ver = buf;
					}
				}
			}
			SetDlgItemTextW(hWnd, IDC_PROG_VER, ver.c_str());
			return TRUE;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hWnd, LOWORD(wParam));
				return TRUE;
			}
			break;
		}
		return FALSE;
	}

	std::wstring m_textVoice;
};

static void AppOnAppAbout()
{
	std::wstring textVoice = L"Текущий диктор: ";
	textVoice += MultiSpeech::getInstance().GetCurrentReader();
	CAboutDlg aboutDlg(textVoice);
	aboutDlg.DoModal(g_hWndMain);
}

// main window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		// remember the main window handle: the App* handlers and the WndProc
		// itself reach the game view through it
		g_hWndMain = hWnd;
		// create the main menu
		g_hMainMenu = LoadMenuW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDR_MAINFRAME));
		SetMenu(hWnd, g_hMainMenu);
		// create the game view controller and attach it to this window
		CPlainInsteadView::CreateView(hWnd);
		return 0;
	}
	case WM_SIZE:
		if (GetView(hWnd))
			GetView(hWnd)->OnSize(LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_SETFOCUS:
		if (GetView(hWnd))
			GetView(hWnd)->OnMainSetFocus();
		return 0;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if (GetView(hWnd))
		{
			GetView(hWnd)->OnCtlColor((HWND)lParam, (HDC)wParam,
				message == WM_CTLCOLOREDIT ? CTLCOLOR_EDIT : CTLCOLOR_STATIC);
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		}
		break;
	case WM_COMMAND:
	{
		// forward control notifications to the view
		if (GetView(hWnd) &&
			GetView(hWnd)->HandleCommand(hWnd, wParam, lParam))
			return 0;
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case ID_APP_ABOUT:
			AppOnAppAbout();
			break;
		case ID_FILE_OPEN:
			AppOnFileOpen();
			break;
		case ID_FILE_SAVE_GAME:
			AppOnFileSave();
			break;
		case ID_NEW_GAME_FROM_FILE:
			AppNewGameFromFile();
			break;
		case ID_NEW_GAME:
			AppOnNewGameFromLib();
			break;
		case ID_RESTART_MENU:
			AppOnRestartMenu();
			break;
		case ID_ENTER_SETUP:
			AppOnEnterSetup();
			break;
		case ID_RESET_ALL_SETTINGS:
			AppOnResetAllSettings();
			break;
		case ID_APP_EXIT:
			AppOnAppExit();
			break;
		case ID_VOLUME_DOWN:
			AppOnVolumeDown();
			break;
		case ID_VOLUME_UP:
			AppOnVolumeUp();
			break;
		case ID_VOLUME_ON:
			AppOnVolumeOn();
			break;
		case ID_LISTSND_ON:
			AppOnListsndOn();
			break;
		case ID_LISTSND_DOWN:
			AppOnListsndDown();
			break;
		case ID_LISTSND_UP:
			AppOnListsndUp();
			break;
		case ID_ADD_GAME_TO_LIB:
			AppOnAddGameToLib();
			break;
		case ID_OPEN_MANAGER:
			AppOnOpenManager();
			break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
		return 0;
	}
	case WM_CLOSE:
	{
		if (!GlobalManager::getInstance().isUserSaveLastFile() && !GlobalManager::getInstance().isIgnoreExitDialog)
		{
			if (MessageBoxW(hWnd, L"Вы уверены что хотите выйти без сохранения игры?", L"Файл не сохранен", MB_YESNOCANCEL) != IDYES)
			{
				return 0;
			}
		}

		// save the window placement
		WINDOWPLACEMENT wp;
		wp.length = sizeof(wp);
		GetWindowPlacement(hWnd, &wp);
		CIniFile mainSettings;
		mainSettings.WriteNumber(L"MainFrame", L"WPlen", (INT)wp.length);
		mainSettings.WriteStruct(L"MainFrame", L"WP", &wp, wp.length);

		InterpreterController::endInterpreter();
		AppQuit();
		return 0;
	}
	case WM_DESTROY:
	{
		// release the view controller attached in WM_CREATE
		CPlainInsteadView* pView = GetView(hWnd);
		if (pView)
			pView->DestroyView();
		g_hWndMain = NULL;
		PostQuitMessage(0);
		return 0;
	}
	default:
		// WM_FINDREPLACE is a RegisterWindowMessage value, not a constant:
		// it must be handled outside the switch
		if (message == WM_FINDREPLACE && GetView(hWnd))
			return GetView(hWnd)->OnFindReplaceMessage(lParam);
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	// make the exe directory current: the engine and BASS use relative paths (tiny.lua, chorium.sf2, temp, games)
	{
		wchar_t exeDir[MAX_PATH];
		GetModuleFileNameW(NULL, exeDir, MAX_PATH);
		wchar_t* slash = wcsrchr(exeDir, L'\\');
		if (slash)
		{
			*slash = 0;
			SetCurrentDirectoryW(exeDir);
		}
	}

	// initialize common controls (for the list view / tab controls)
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	// initialize BASS.dll
	gBassInit = 1;
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		gBassInit = 0;
	}

	// NOTE: BASS_DEVICE_DEFAULT == BASS_DEVICE_MONO (both are 2), so the old
	// BASS_DEVICE_DEFAULT|BASS_DEVICE_FREQ silently initialized a MONO mixer;
	// device -1 is the default anyway, keep only the freq flag => stereo
	if (BASS_Init(-1, 44100, BASS_DEVICE_FREQ, 0, NULL) == 0) {
		gBassInit = 0;
	}

	// подключаем плагин миди
	BASS_PluginLoad("bassmidi.dll", 0);
	// шрифт для миди
	HSOUNDFONT newfont = BASS_MIDI_FontInit("chorium.sf2", 0);
	if (newfont) {
		BASS_MIDI_FONT sf;
		sf.font = newfont;
		sf.preset = -1; // use all presets
		sf.bank = 0; // use default bank(s)
		BASS_MIDI_StreamSetFonts(0, &sf, 1);    // set default soundfont
	}

	// установка текущей локали
	LPCWSTR lpsz = _wsetlocale(LC_CTYPE, L"rus");
	if (0 == lpsz)
	{
		fwprintf(stderr, L"Failed set locale\n");
	}

	/* инициализация движка для LUA */
	ext.init = tiny_init;

	if (instead_extension(&ext)) {
		fwprintf(stderr, L"Failed set tiny\n");
	}

	// звуковая подсистема LUA
	instead_sound_init();

	// register the main window class
	WNDCLASSEXW wcex;
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_PLAININSTEAD));
	wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_PLAININSTEAD));
	wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = L"PlainInsteadWndClass";
	if (!RegisterClassExW(&wcex))
	{
		MessageBoxW(NULL, L"Call to RegisterClassEx failed!", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	// read the saved window placement
	WINDOWPLACEMENT wp;
	memset(&wp, 0, sizeof(wp));
	wp.length = sizeof(wp);
	wp.showCmd = nCmdShow;
	{
		CIniFile mainSettings;
		UINT nl = mainSettings.GetInt(L"MainFrame", L"WPlen", 0);
		if (nl && nl <= sizeof(WINDOWPLACEMENT))
		{
			WINDOWPLACEMENT wpSaved;
			memset(&wpSaved, 0, sizeof(wpSaved));
			if (mainSettings.GetStruct(L"MainFrame", L"WP", &wpSaved, nl) && wpSaved.length == nl)
			{
				wp = wpSaved;
			}
		}
	}

	// create the main window
	HWND hWnd = CreateWindowExW(0, L"PlainInsteadWndClass", L"PlainInstead",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
	{
		MessageBoxW(NULL, L"Call to CreateWindow failed!", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	g_hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_MAINFRAME));

	// open the game from the command line, or show the launcher
	if (lpCmdLine && lpCmdLine[0])
	{
		// strip quotes
		std::wstring file = lpCmdLine;
		if (!file.empty() && file[0] == L'"')
		{
			file = file.substr(1);
			size_t q = file.find(L'"');
			if (q != std::wstring::npos) file = file.substr(0, q);
		}
		currFilePath = file;
		currFileName = file.substr(file.find_last_of(L'\\') + 1);
		SetWindowPlacement(hWnd, &wp);
		ShowWindow(hWnd, wp.showCmd);
		UpdateWindow(hWnd);
		AppStartNewGameFile(currFilePath, currFileName);
	}
	else
	{
		SetWindowPlacement(hWnd, &wp);
		ShowWindow(hWnd, wp.showCmd);
		UpdateWindow(hWnd);
		AppOnOpenManager();
	}

	// main message loop
	HACCEL hAccelTable = g_hAccel;
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		// accelerators must be translated against the MAIN window (owner of the menu),
		// not msg.hwnd: child controls cannot dispatch menu commands
		if (hAccelTable && TranslateAcceleratorW(hWnd, hAccelTable, &msg))
			continue;
		// Tab navigation among the controls owned by the main window;
		// (Enter in the lists is handled by the list subclass procedures)
		if (msg.hwnd && (msg.hwnd == hWnd || IsChild(hWnd, msg.hwnd)))
		{
			// Tab must move focus between the game lists: a multiline edit
			// control claims DLGC_WANTTAB, so IsDialogMessageW would feed Tab
			// to the readonly editor where it does nothing (focus trap)
			if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_TAB))
			{
				HWND from = GetFocus();
				if (!from || !IsChild(hWnd, from))
					from = hWnd;
				HWND next = GetNextDlgTabItem(hWnd, from, GetKeyState(VK_SHIFT) < 0);
				if (next)
				{
					SetFocus(next);
					continue;
				}
			}
			// IsDialogMessageW with the main window would eat Enter for the
			// default button; only let it handle navigation keys
			if ((msg.message == WM_KEYDOWN) &&
				(msg.wParam == VK_UP || msg.wParam == VK_DOWN ||
				 msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
			{
				if (IsDialogMessageW(hWnd, &msg))
					continue;
			}
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return (int)msg.wParam;
}
