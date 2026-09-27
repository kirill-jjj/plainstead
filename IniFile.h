// IniFile.h: simple wrapper over the Windows INI file API (WritePrivateProfileString / GetPrivateProfileString)
//

#pragma once

#include "stdafx.h"

class CIniFile
{
public:
	CIniFile(LPCTSTR lpIniFileName, INT iMaxStringLength);
	CIniFile(); // opens settings.ini next to the executable
	virtual ~CIniFile();

protected:
	std::wstring m_strFileName;	// path to the INI file
	const INT	m_MAXSTRLEN;	// max length of a string (excluding the key name) that can be written/read to/from the INI file by this instance

// Implementation
public:
	std::wstring GetIniFileName() const;
	void	SetIniFileName(LPCTSTR lpIniFileName);

	BOOL	GetString(LPCTSTR lpSection, LPCTSTR lpKey, std::wstring& strRet, LPCTSTR strDefault);
	UINT	GetInt(LPCTSTR lpSection, LPCTSTR lpKey, INT iDefaultValue);
	FLOAT	GetFloat(LPCTSTR lpSection, LPCTSTR lpKey, FLOAT fDefaultValue);
	BOOL	GetStruct(LPCTSTR lpSection, LPCTSTR lpKey, LPVOID lpRetStruct, UINT iSizeStruct);
	void	GetSectionNames(std::vector<std::wstring>& lstSectionNames);

	BOOL	WriteSection(LPCTSTR lpSection, LPCTSTR lpData);
	BOOL	WriteString(LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpString);
	BOOL	WriteNumber(LPCTSTR lpSection, LPCTSTR lpKey, INT iValue);
	BOOL	WriteNumber(LPCTSTR lpSection, LPCTSTR lpKey, FLOAT fValue);
	BOOL	WriteStruct(LPCTSTR lpSection, LPCTSTR lpKey, LPVOID lpStruct, UINT iSizeStruct);

	BOOL	RemoveKey(LPCTSTR lpSection, LPCTSTR lpKey);
};
