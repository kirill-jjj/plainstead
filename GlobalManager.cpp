// GlobalManager.cpp: implementation of the GlobalManager class
//

#include "stdafx.h"
#include "GlobalManager.h"

int GlobalManager::lastString = 0;

GlobalManager::GlobalManager() :
	m_haveFileSaved(true),
	m_userStartGame(false),
	m_useMenu(false),
	m_en_history(true)
{
	m_currHistPos = -1;
	m_currCmdPos = -1;
}

GlobalManager::~GlobalManager()
{
}

void GlobalManager::userStartGame()
{
	m_userStartGame = true;
	// reset the history
	commandHistory.clear();
	m_currHistPos = -1;
	m_currCmdPos = -1;
	// add an empty command (root of the history)
	appendCommand(L"");
}

void GlobalManager::userSavedFile()
{
	m_haveFileSaved = true;
}

void GlobalManager::userNewCommand()
{
	m_haveFileSaved = false;
}

bool GlobalManager::isUserSaveLastFile()
{
	return m_haveFileSaved;
}

bool GlobalManager::isUserStartGame()
{
	return m_userStartGame;
}

void GlobalManager::appendCommandAndRespond(std::wstring command, std::wstring respond)
{
	commandHistory.push_back(std::make_pair(command, respond));
	m_currHistPos = (int)commandHistory.size() - 1;
	m_currCmdPos = (int)commandHistory.size() - 1;
}

void GlobalManager::enableHistory(bool isEn)
{
	m_en_history = isEn;
}

void GlobalManager::appendCommand(std::wstring command)
{
	if (!m_en_history) return;
	// append a new command to the end of the history
	commandHistory.push_back(std::make_pair(command, L""));
	// move to the last element of the history
	m_currHistPos = (int)commandHistory.size() - 1;
	m_currCmdPos = (int)commandHistory.size() - 1;
}

void GlobalManager::appendLastRespond(std::wstring command)
{
	if (!m_en_history) return;
	// attach the latest response
	commandHistory[m_currHistPos].second = command;
}

std::wstring GlobalManager::fullHistoryData()
{
	if (commandHistory.size() > 0)
	{
		std::wstring hist;
		hist.append(L"История\r\n");
		for (size_t i = 0; i < commandHistory.size(); i++)
		{
			std::wstring res;
			res.append(L"> ");
			res.append(commandHistory[i].first);
			res.append(L"\r\n");
			res.append(commandHistory[i].second);
			hist.append(res);
		}
		return hist;
	}
	return std::wstring();
}

std::wstring GlobalManager::previosHistoryData()
{
	if (previosHistoryHave())
	{
		m_currHistPos--;
		std::wstring res;
		res.resize(128);
		swprintf_s(&res[0], res.size(), L"Ход: %d из %d\r\n", m_currHistPos + 1, (int)commandHistory.size());
		res.resize(wcslen(res.c_str()));
		res.append(L"> ");
		res.append(commandHistory[m_currHistPos].first);
		res.append(L"\r\n");
		res.append(commandHistory[m_currHistPos].second);
		return res;
	}
	return std::wstring();
}

bool GlobalManager::previosHistoryHave()
{
	return (m_currHistPos > 0);
}

std::wstring GlobalManager::nextHistoryData()
{
	if (nextHistoryHave())
	{
		m_currHistPos++;
		std::wstring res;
		res.resize(128);
		swprintf_s(&res[0], res.size(), L"Ход: %d из %d\r\n", m_currHistPos + 1, (int)commandHistory.size());
		res.resize(wcslen(res.c_str()));
		res.append(L"> ");
		res.append(commandHistory[m_currHistPos].first);
		res.append(L"\r\n");
		res.append(commandHistory[m_currHistPos].second);
		return res;
	}
	return std::wstring();
}

bool GlobalManager::nextHistoryHave()
{
	return ((m_currHistPos >= 0) && (m_currHistPos < (int)(commandHistory.size() - 1)));
}

std::wstring GlobalManager::commandData()
{
	if (commandHistory.size() > 0 && m_currCmdPos >= 0)
	{
		return commandHistory[m_currCmdPos].first;
	}
	return std::wstring();
}

bool GlobalManager::previosCommandMove()
{
	if (m_currCmdPos > 1)
	{
		m_currCmdPos--;
		return true;
	}
	return false;
}

bool GlobalManager::nextCommandMove()
{
	if ((m_currCmdPos >= 0) && (m_currCmdPos < (int)(commandHistory.size() - 1)))
	{
		m_currCmdPos++;
		return true;
	}
	return false;
}


bool GlobalManager::isUseMenu()
{
	return m_useMenu;
}

bool GlobalManager::isAutoMenuDetect()
{
	return true;
}

std::wstring GlobalManager::keyMenuString()
{
	return L"Что вы хотите сделать дальше?";
}

void GlobalManager::setUseMenu()
{
	m_useMenu = true;
}
