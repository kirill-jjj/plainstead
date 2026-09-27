// PlainInstead.h : application-level functions (pure Win32)
//

#pragma once

#include "stdafx.h"

// utf8 <-> wide conversion helpers (defined in PlainInstead.cpp)
std::string utf8_encode(const std::wstring &wstr);
std::wstring utf8_decode(const char* utf8Str);

// exe directory (with a trailing backslash)
std::wstring GetExeDir();

// main menu handle (used to check/uncheck menu items)
HMENU AppGetMainMenu();
// the shared output font
HFONT AppGetOutFont();
void AppSetOutFont(HFONT hFont);
// the saves directory of the current game
std::wstring AppGetSaveDir();

// application actions (were CPlainInsteadApp methods)
void AppStartNewGameFile(const std::wstring& file, const std::wstring& name);
void AppOnFileOpen();
void AppOnFileSave();
void AppOnNewGameFromLib();
