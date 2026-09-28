// StdioFileEx.cpp: implementation of the light CStdioFileEx (pure Win32, no MFC)
//

#include "stdafx.h"
#include "StdioFileEx.h"

const UINT CStdioFileEx::modeWriteUnicode = 0x200000;

static const unsigned char UTF8_BOM[] = { 0xEF, 0xBB, 0xBF };

CStdioFileEx::CStdioFileEx() :
	m_pFile(NULL),
	m_bIsUnicodeText(false),
	m_nOpenFlags(0),
	m_nCodePage(CP_UTF8),
	m_bWriteBOM(true),
	m_bBomWritten(false)
{
}

CStdioFileEx::CStdioFileEx(LPCTSTR lpszFileName, UINT nOpenFlags) :
	m_pFile(NULL),
	m_bIsUnicodeText(false),
	m_nOpenFlags(0),
	m_nCodePage(CP_UTF8),
	m_bWriteBOM(true),
	m_bBomWritten(false)
{
	Open(lpszFileName, nOpenFlags);
}

CStdioFileEx::~CStdioFileEx()
{
	Close();
}

BOOL CStdioFileEx::Open(LPCTSTR lpszFileName, UINT nOpenFlags)
{
	Close();

	m_nOpenFlags = nOpenFlags;
	// strip the text flag, stdio handles newlines itself
	UINT flags = nOpenFlags & ~OpenFlags::text;
	flags |= OpenFlags::binary;

	TCHAR mode[8];
	if (flags & OpenFlags::create)
		wcscpy_s(mode, (flags & OpenFlags::read) ? L"w+b" : L"wb");
	else if ((flags & (OpenFlags::read | OpenFlags::write)) == (OpenFlags::read | OpenFlags::write))
		wcscpy_s(mode, L"r+b"); // read+write on an existing file: do NOT truncate
	else if ((flags & OpenFlags::write) && (flags & OpenFlags::noTruncate))
		wcscpy_s(mode, L"ab");
	else if (flags & OpenFlags::write)
		wcscpy_s(mode, L"wb");
	else
		wcscpy_s(mode, L"rb");

	m_pFile = _wfopen(lpszFileName, mode);
	if (!m_pFile)
		return FALSE;

	// detect a UTF-16 BOM when reading an existing file
	if ((flags & OpenFlags::read) && !(flags & OpenFlags::create))
	{
		wint_t c1 = fgetc(m_pFile);
		wint_t c2 = fgetc(m_pFile);
		if (c1 == 0xFF && c2 == 0xFE)
		{
			m_bIsUnicodeText = true;
		}
		else if (c1 != WEOF)
		{
			ungetc(c2, m_pFile);
			ungetc(c1, m_pFile);
		}
	}

	return TRUE;
}

void CStdioFileEx::Close()
{
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
	m_bIsUnicodeText = false;
	m_bBomWritten = false;
}

ULONGLONG CStdioFileEx::SeekToEnd()
{
	if (!m_pFile) return 0;
	fseek(m_pFile, 0, SEEK_END);
	return _ftelli64(m_pFile);
}

ULONGLONG CStdioFileEx::Seek(LONGLONG lOff, UINT nFrom)
{
	if (!m_pFile) return 0;
	_fseeki64(m_pFile, lOff, (int)nFrom);
	return _ftelli64(m_pFile);
}

ULONGLONG CStdioFileEx::GetLength() const
{
	if (!m_pFile) return 0;
	__int64 curr = _ftelli64(m_pFile);
	_fseeki64(m_pFile, 0, SEEK_END);
	__int64 len = _ftelli64(m_pFile);
	_fseeki64(m_pFile, (long)curr, SEEK_SET);
	return (ULONGLONG)len;
}

// reads one UTF-8 sequence (1..4 bytes) from the file
bool CStdioFileEx::ReadUtf8Char(std::string& out)
{
	int c = fgetc(m_pFile);
	if (c == EOF) return false;
	out.clear();
	out.push_back((char)c);
	int extra = 0;
	if ((c & 0xE0) == 0xC0) extra = 1;
	else if ((c & 0xF0) == 0xE0) extra = 2;
	else if ((c & 0xF8) == 0xF0) extra = 3;
	for (int i = 0; i < extra; i++)
	{
		c = fgetc(m_pFile);
		if (c == EOF) return false;
		out.push_back((char)c);
	}
	return true;
}

// reads one UTF-16 code unit (with surrogate pairing) from the file
bool CStdioFileEx::ReadUtf16Char(std::wstring& out)
{
	wint_t c1 = fgetc(m_pFile);
	wint_t c2 = fgetc(m_pFile);
	if (c1 == EOF || c2 == EOF) return false;
	wchar_t ch = (wchar_t)((c2 << 8) | c1); // little-endian BOM
	out.clear();
	out.push_back(ch);
	if (ch >= 0xD800 && ch <= 0xDBFF) // high surrogate: read the low one
	{
		wint_t c3 = fgetc(m_pFile);
		wint_t c4 = fgetc(m_pFile);
		if (c3 != EOF && c4 != EOF)
		{
			out.push_back((wchar_t)((c4 << 8) | c3));
		}
	}
	return true;
}

BOOL CStdioFileEx::ReadString(std::wstring& rString)
{
	rString.clear();
	if (!m_pFile) return FALSE;

	std::wstring line;
	bool bAny = false;
	while (true)
	{
		std::wstring ch;
		if (m_bIsUnicodeText)
		{
			if (!ReadUtf16Char(ch)) break;
		}
		else
		{
			std::string ch8;
			if (!ReadUtf8Char(ch8)) break;
			// convert UTF-8 to UTF-16
			int need = MultiByteToWideChar(m_nCodePage, 0, ch8.c_str(), (int)ch8.size(), NULL, 0);
			if (need > 0)
			{
				std::vector<wchar_t> buf(need);
				MultiByteToWideChar(m_nCodePage, 0, ch8.c_str(), (int)ch8.size(), &buf[0], need);
				ch.assign(&buf[0], need);
			}
		}
		bAny = true;
		if (ch == L"\r")
			continue; // handled with the \n
		if (ch == L"\n")
			break; // end of line
		line += ch;
	}
	rString = line;
	return bAny ? TRUE : FALSE;
}

void CStdioFileEx::WriteString(LPCTSTR lpsz)
{
	if (!m_pFile || !lpsz) return;

	// write the BOM once, at the start of the file
	if (!m_bBomWritten && m_bWriteBOM)
	{
		if (m_nOpenFlags & modeWriteUnicode)
		{
			wchar_t cBOM = 0xFEFF;
			fwrite(&cBOM, sizeof(wchar_t), 1, m_pFile);
		}
		else if (m_nCodePage == CP_UTF8)
		{
			fwrite(UTF8_BOM, sizeof(UTF8_BOM), 1, m_pFile);
		}
		m_bBomWritten = true;
	}

	if (m_nOpenFlags & modeWriteUnicode)
	{
		// UTF-16 LE output
		size_t len = wcslen(lpsz);
		std::vector<wchar_t> text(len);
		for (size_t i = 0; i < len; i++)
			text[i] = lpsz[i];
		fwrite(&text[0], sizeof(wchar_t), len, m_pFile);
	}
	else if (m_nCodePage == CP_UTF8)
	{
		int need = WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, NULL, 0, NULL, NULL);
		if (need > 1)
		{
			std::vector<char> buf(need);
			WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, &buf[0], need, NULL, NULL);
			fwrite(&buf[0], 1, need - 1, m_pFile); // skip the terminating zero
		}
	}
	else
	{
		// ANSI code page
		int need = WideCharToMultiByte(m_nCodePage, 0, lpsz, -1, NULL, 0, NULL, NULL);
		if (need > 1)
		{
			std::vector<char> buf(need);
			WideCharToMultiByte(m_nCodePage, 0, lpsz, -1, &buf[0], need, NULL, NULL);
			fwrite(&buf[0], 1, need - 1, m_pFile);
		}
	}
}
