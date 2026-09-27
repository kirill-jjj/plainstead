// GlobalManager.h: command / response history and misc global state
//

#pragma once

#include "stdafx.h"
#include <vector>

class GlobalManager
{
public:
	static GlobalManager& getInstance() {
		static GlobalManager instance;
		return instance;
	}
	void userStartGame(); // user started a game
	void userSavedFile();  // user saved the game
	void userNewCommand(); // a new command was issued
	bool isUserSaveLastFile(); // true when the last command was a save
	bool isUserStartGame();
	void appendCommandAndRespond(std::wstring command, std::wstring respond); // append command and response to the history
	void appendCommand(std::wstring command); // append a new command
	void appendLastRespond(std::wstring command); // attach the latest response to the last command
	std::wstring fullHistoryData(); // full history (command+response)
	std::wstring previosHistoryData(); // step back in the history (command+response)
	bool previosHistoryHave(); // is a previous step available
	std::wstring nextHistoryData(); // step forward in the history (command+response)
	bool nextHistoryHave(); // is a next step available
	void enableHistory(bool isEn); // enable/disable the history

	std::wstring commandData(); // current command
	bool previosCommandMove(); // step to the previous command
	bool nextCommandMove(); // step to the next command

	bool isIgnoreExitDialog; // suppress the exit dialog
	bool isUseMenu(); // the menu is used
	bool isAutoMenuDetect(); // auto-detect the menu
	std::wstring keyMenuString(); // menu detection key string
	void setUseMenu();
	static int lastString;
private:
	GlobalManager();
	~GlobalManager();
	GlobalManager(const GlobalManager&);
	GlobalManager& operator=(const GlobalManager&);

	bool m_haveFileSaved;
	bool m_userStartGame;
	std::vector< std::pair<std::wstring/*command*/, std::wstring/*respond*/> > commandHistory; // command history
	int m_currHistPos; // current position in the history
	int m_currCmdPos; // current position in the command list
	bool m_useMenu;
	bool m_en_history;
};
