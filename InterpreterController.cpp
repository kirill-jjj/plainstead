// InterpreterController.cpp: implementation of the InterpreterController class
//

#include "stdafx.h"
#include "InterpreterController.h"
#include <string>

extern "C" {
#include "instead/instead.h"
}

std::wstring InterpreterController::m_gameFile = L"";
std::wstring InterpreterController::m_lastCommand = L"";
bool InterpreterController::m_wasCommand = false;

InterpreterController::InterpreterController(void)
{
}


InterpreterController::~InterpreterController(void)
{
}

static std::string utf8_from_wide(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

static std::wstring utf8_to_wide(const char* s)
{
	if (!s || !*s) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	std::wstring wTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, s, -1, &wTo[0], size_needed);
	if (!wTo.empty() && wTo.back() == 0) wTo.pop_back();
	return wTo;
}

void InterpreterController::startGameFile(const std::wstring& gameFile, const std::wstring& gameName, int autolog)
{
	startGameFile(gameFile, gameName, L"", autolog);
}

void InterpreterController::startGameFile(const std::wstring& gameFile, const std::wstring& gameName, const std::wstring& saveFile, int autolog)
{
	// TODO: run the game in a thread
	std::string ascii = utf8_from_wide(gameFile);
	instead_done();
	instead_set_debug(1);
	if (instead_init(ascii.c_str()) != 0)
	{
		MessageBoxW(NULL, (L"Не удалось инициализировать игру:\n" + gameFile + L"\n\n" + utf8_to_wide(instead_err())).c_str(),
			L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}
	{
		char *str;
		if (instead_load(&str) == 0)
		{
			m_gameFile = gameFile;
			m_lastCommand = L"";
			m_wasCommand = true;
		}
		else
		{
			MessageBoxW(NULL, (L"Не удалось загрузить игру:\n" + gameFile + L"\n\n" + utf8_to_wide(instead_err())).c_str(),
				L"Ошибка", MB_OK | MB_ICONERROR);
		}
	}
}

std::wstring InterpreterController::RunInterpreter(const std::wstring& command)
{
	std::wstring Result;

	// save the command, but do not execute it here
	m_lastCommand = command;
	m_wasCommand = true;

	return Result;
}

void InterpreterController::endInterpreter()
{
}

// run the interpreter with a save file
bool InterpreterController::loadSave(const std::wstring& fname)
{
	startGameFile(m_gameFile, fname, 1);
	return true;
}

bool InterpreterController::saveGame(const std::wstring& fname)
{
	return CopyFileW(L"temp\\last.sav", fname.c_str(), FALSE);
}

std::wstring InterpreterController::lastCommand()
{
	return m_lastCommand;
}

void InterpreterController::clearNewCommandFlag()
{
	m_wasCommand = false;
}


bool InterpreterController::wasNewCommand()
{
	return m_wasCommand;
}
