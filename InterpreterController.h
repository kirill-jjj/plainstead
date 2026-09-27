#pragma once

#include "stdafx.h"

class InterpreterController
{
public:
	InterpreterController(void);
	~InterpreterController(void);
	static void startGameFile(const std::wstring& gameFile, const std::wstring& gameName, int autolog); // start the game from a file
	static std::wstring RunInterpreter(const std::wstring& command); // start the interpreter; the command is remembered but not executed
	static void endInterpreter();
	static bool loadSave(const std::wstring& fname); // load a save
	static bool saveGame(const std::wstring& fname); // save the game
	static bool wasNewCommand(); // is there a new command to process
	static void clearNewCommandFlag(); // reset the new command flag
	static std::wstring lastCommand(); // the last command
private:
	static void startGameFile(const std::wstring& gameFile, const std::wstring& gameName, const std::wstring& saveFile, int autolog);
private:
	static std::wstring m_gameFile;
	static std::wstring m_lastCommand;
	static bool m_wasCommand;
};
