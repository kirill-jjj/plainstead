// StdioFileEx.h: a light stdio-based text file wrapper (pure Win32, no MFC).
// Supports the subset of the old MFC CStdioFileEx API used by this project:
//   Open / Close / ReadString / WriteString / SetCodePage / SeekToEnd / GetLength
// Flags are compatible: CFile::modeRead / modeWrite / modeCreate / modeNoTruncate /
//   typeBinary / typeText and CStdioFileEx::modeWriteUnicode.
//

#pragma once

#include "stdafx.h"

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

// CFile-compatible flag subset
namespace CFile
{
	const UINT modeRead = 0x00000001;
	const UINT modeWrite = 0x00000002;
	const UINT modeReadWrite = 0x00000003;
	const UINT modeCreate = 0x00001000;
	const UINT modeNoTruncate = 0x00008000;
	const UINT typeBinary = 0x00010000;
	const UINT typeText = 0x00020000;
}

class CStdioFileEx
{
public:
	static const UINT modeWriteUnicode;

	CStdioFileEx();
	CStdioFileEx(LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual ~CStdioFileEx();

	BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags);
	void Close();
	BOOL IsOpen() const { return m_pFile != NULL; }

	BOOL ReadString(std::wstring& rString);
	void WriteString(LPCTSTR lpsz);

	void SetCodePage(UINT nCodePage) { m_nCodePage = nCodePage; }
	void SetWriteBOM(bool bWrite) { m_bWriteBOM = bWrite; }

	ULONGLONG SeekToEnd();
	ULONGLONG Seek(LONGLONG lOff, UINT nFrom); // nFrom: SEEK_SET / SEEK_CUR / SEEK_END
	ULONGLONG GetLength() const;

private:
	CStdioFileEx(const CStdioFileEx&);
	CStdioFileEx& operator=(const CStdioFileEx&);

	bool ReadUtf8Char(std::string& out); // reads one UTF-8 sequence
	bool ReadUtf16Char(std::wstring& out); // reads one UTF-16 code unit (with surrogate pairing)

	FILE* m_pFile;
	bool m_bIsUnicodeText;
	UINT m_nOpenFlags;
	UINT m_nCodePage;
	bool m_bWriteBOM;
	bool m_bBomWritten;
};
