// IniFile.cpp: implementation of the CIniFile class (pure Win32)
//

#include "stdafx.h"
#include "IniFile.h"

/////////////////////////////////////////////////////////////////////////////

CIniFile::CIniFile(LPCTSTR	strIniFileName,
				   INT		iMaxStringLength)
	: m_MAXSTRLEN(iMaxStringLength)
{
	SetIniFileName(strIniFileName);
}

CIniFile::CIniFile() : m_MAXSTRLEN(1024)
{
	TCHAR buff[MAX_PATH];
	memset(buff, 0, sizeof(buff));
	::GetModuleFileNameW(NULL, buff, MAX_PATH);
	std::wstring baseDir = buff;
	baseDir = baseDir.substr(0, baseDir.find_last_of(L'\\') + 1);
	SetIniFileName((baseDir + L"\\settings.ini").c_str());
}


CIniFile::~CIniFile()
{
}


// PURPOSE: Get the name of the INI file.
std::wstring CIniFile::GetIniFileName() const
{
	return m_strFileName;
}

// PURPOSE: Set the name of the INI file.
void
CIniFile::SetIniFileName(LPCTSTR lpIniFileName)
{
	m_strFileName = lpIniFileName;
}


// PURPOSE:	Create a new section.
// NOTE:	If the INI file doesn't exist, this method creates it.
BOOL
CIniFile::WriteSection(LPCTSTR lpSection,
					   LPCTSTR lpData)
{
	return ::WritePrivateProfileSection(lpSection, lpData, m_strFileName.c_str());
}

// PURPOSE:	Write string data to the INI file
BOOL
CIniFile::WriteString(LPCTSTR lpSection,
					  LPCTSTR lpKey,
					  LPCTSTR lpString)
{
	return ::WritePrivateProfileString(lpSection, lpKey, lpString, m_strFileName.c_str());
}

// PURPOSE:	Write an integer (signed) to the INI file
BOOL
CIniFile::WriteNumber(LPCTSTR lpSection,
					  LPCTSTR lpKey,
					  INT iValue)
{
	TCHAR str[32];
	swprintf_s(str, L"%d", iValue);
	return WriteString(lpSection, lpKey, str);
}

// PURPOSE:	Write the data into the specified key in the INI file.
BOOL
CIniFile::WriteStruct(LPCTSTR lpSection,
					  LPCTSTR lpKey,
					  LPVOID lpStruct,
					  UINT iSizeStruct)
{
	return ::WritePrivateProfileStruct(lpSection, lpKey, lpStruct, iSizeStruct, m_strFileName.c_str());
}

// PURPOSE:	Write a float to the INI file.
BOOL
CIniFile::WriteNumber(LPCTSTR	lpSection,
					  LPCTSTR	lpKey,
					  FLOAT		fValue)
{
	TCHAR str[64];
	swprintf_s(str, L"%f", fValue);
	return WriteString(lpSection, lpKey, str);
}

// PURPOSE:	Remove a key from a specified section.
BOOL
CIniFile::RemoveKey(LPCTSTR lpSection,
					LPCTSTR lpKey)
{
	return WriteString(lpSection, lpKey, NULL);
}


// PURPOSE:	Read an integer from the INI file.
UINT
CIniFile::GetInt(LPCTSTR lpSection,
				 LPCTSTR lpKey,
				 const INT iDefaultValue)
{
	return ::GetPrivateProfileInt(lpSection, lpKey, iDefaultValue, m_strFileName.c_str());
}

// PURPOSE:	Read a string from the INI file.
BOOL
CIniFile::GetString(LPCTSTR lpSection,
					LPCTSTR lpKey,
					std::wstring& strRet,
					LPCTSTR lpDefault)
{
	std::vector<TCHAR> buf(m_MAXSTRLEN);
	DWORD iRet = ::GetPrivateProfileString(lpSection, lpKey, lpDefault, &buf[0], m_MAXSTRLEN, m_strFileName.c_str());
	strRet.assign(&buf[0], iRet);
	return (iRet > 0);
}

// PURPOSE:	Read a float from the INI file.
FLOAT
CIniFile::GetFloat(LPCTSTR lpSection,
				   LPCTSTR lpKey,
				   const FLOAT fDefaultValue)
{
	std::wstring strRet;
	TCHAR strDefault[64];
	swprintf_s(strDefault, L"%f", fDefaultValue);
	BOOL bRet = GetString(lpSection, lpKey, strRet, strDefault);
	return (bRet ? (FLOAT)_wtof(strRet.c_str()) : fDefaultValue);
}

// PURPOSE:	Read a struct from the INI file
BOOL
CIniFile::GetStruct(LPCTSTR lpSection,
					LPCTSTR lpKey,
					LPVOID	lpRetStruct,
					const UINT iSizeStruct)
{
	return ::GetPrivateProfileStruct(lpSection, lpKey, lpRetStruct, iSizeStruct, m_strFileName.c_str());
}

// PURPOSE:	Get the list of section names from the INI file.
void
CIniFile::GetSectionNames(std::vector<std::wstring>& lstSectionNames)
{
	lstSectionNames.clear();
	std::vector<TCHAR> lpRetBuff(m_MAXSTRLEN);
	::GetPrivateProfileSectionNames(&lpRetBuff[0], m_MAXSTRLEN, m_strFileName.c_str());

	// Parse out the individual names and store them in the list
	for (LPCTSTR p = &lpRetBuff[0]; *p != L'\0'; p += wcslen(p) + 1)
	{
		lstSectionNames.push_back(p);
	}
}
