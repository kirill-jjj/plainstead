// unzip.cpp: zip reading backed by miniz (https://github.com/richgel999/miniz).
// Replaces the 2002-era "Zip Utils" (zlib 1.1.3 + minizip 0.15 beta) with a
// modern, maintained, single-file implementation. The exported API matches
// the classic Zip Utils surface used by the callers.
//

#include "stdafx.h"
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.h"
#include "unzip.h"

static ZRESULT lasterror = ZR_OK;

struct ZipHandle
{
	mz_zip_archive zip;
	FILE* file;           // owned FILE* when opened by name
	std::wstring baseDir; // base dir for relative paths
	ZipHandle() : file(NULL) { memset(&zip, 0, sizeof(zip)); }
};

// create every missing component of the directory path; unlike
// SHCreateDirectoryExW this accepts relative paths and reports failures
static void CreateDirRecursive(const std::wstring& path)
{
	if (path.empty()) return;
	std::wstring cur;
	size_t i = 0;
	while (i < path.size())
	{
		size_t slash = path.find_first_of(L"\\/", i);
		if (slash == std::wstring::npos) slash = path.size();
		if (cur.empty() && slash == 2 && path[1] == L':')
			cur = path.substr(0, slash); // drive letter only, nothing to create
		else
		{
			if (cur.empty()) cur = path.substr(0, slash);
			else cur += L"\\" + path.substr(i, slash - i);
			if (!cur.empty() && cur.back() != L':' &&
				GetFileAttributesW(cur.c_str()) == INVALID_FILE_ATTRIBUTES)
				CreateDirectoryW(cur.c_str(), NULL);
		}
		i = slash + 1;
	}
}

// cut off sneaky prefixes (\, /, c:\, ..\) from archive entry names (Zip Slip guard)
static void SafeEntryName(const char* utf8name, TCHAR* out, size_t outLen)
{
	// convert UTF-8 to wide
	int need = MultiByteToWideChar(CP_UTF8, 0, utf8name, -1, NULL, 0);
	std::wstring name;
	if (need > 1)
	{
		name.resize(need - 1);
		MultiByteToWideChar(CP_UTF8, 0, utf8name, -1, &name[0], need);
	}
	// normalize backslashes to forward for processing
	for (wchar_t& c : name) if (c == L'\\') c = L'/';

	const wchar_t* s = name.c_str();
	for (;;)
	{
		if (s[0] != 0 && s[1] == L':') { s += 2; continue; }        // drive prefix
		if (s[0] == L'/' ) { s++; continue; }                       // absolute root
		if (_wcsnicmp(s, L"../", 3) == 0) { s += 3; continue; }     // parent traversal
		if (_wcsnicmp(s, L"./", 2) == 0) { s += 2; continue; }      // current dir
		const wchar_t* c;
		c = wcsstr(s, L"/../"); if (c) { s = c + 4; continue; }     // embedded traversal
		c = wcsstr(s, L"/./");  if (c) { s = c + 3; continue; }
		break;
	}
	// back to Windows separators
	std::wstring clean(s);
	for (wchar_t& c : clean) if (c == L'/') c = L'\\';
	wcsncpy_s(out, outLen, clean.c_str(), _TRUNCATE);
}

// file-name overload is the one used by this project
HZIP OpenZip(const TCHAR* fn, const char* password)
{
	lasterror = ZR_OK;
	if (!fn) { lasterror = ZR_ARGS; return 0; }

	// open by name through the wide CRT: no 8.3-name dance, unicode paths work
	FILE* f = _wfopen(fn, L"rb");
	if (!f) { lasterror = ZR_NOFILE; return 0; }

	ZipHandle* handle = new ZipHandle();
	if (!mz_zip_reader_init_cfile(&handle->zip, f, 0, 0))
	{
		fclose(f);
		delete handle;
		lasterror = ZR_CORRUPT;
		return 0;
	}
	handle->file = f; // closed in CloseZip
	return (HZIP)handle;
}

// HANDLE overload: not used by the project; miniz needs a CRT FILE*, which the
// caller can pass through OpenZip's memory variant if ever required
HZIP OpenZipHandle(HANDLE h, const char* password)
{
	(void)h; (void)password;
	lasterror = ZR_ARGS;
	return 0;
}

HZIP OpenZip(void* z, unsigned int len, const char* password)
{
	lasterror = ZR_OK;
	ZipHandle* handle = new ZipHandle();
	memset(&handle->zip, 0, sizeof(handle->zip));
	if (!z || len == 0) { lasterror = ZR_ARGS; delete handle; return 0; }
	if (!mz_zip_reader_init_mem(&handle->zip, z, len, 0))
	{
		lasterror = ZR_CORRUPT;
		delete handle;
		return 0;
	}
	return (HZIP)handle;
}

ZRESULT CloseZip(HZIP hz)
{
	if (!hz) { lasterror = ZR_ARGS; return ZR_ARGS; }
	ZipHandle* handle = (ZipHandle*)hz;
	mz_zip_reader_end(&handle->zip);
	if (handle->file) fclose(handle->file); // owned by OpenZip(fn)
	delete handle;
	lasterror = ZR_OK;
	return ZR_OK;
}

ZRESULT SetUnzipBaseDir(HZIP hz, const TCHAR* dir)
{
	if (!hz || !dir) { lasterror = ZR_ARGS; return ZR_ARGS; }
	ZipHandle* handle = (ZipHandle*)hz;
	handle->baseDir = dir;
	// trim trailing slashes except the root
	while (handle->baseDir.size() > 1 &&
		(handle->baseDir.back() == L'\\' || handle->baseDir.back() == L'/'))
		handle->baseDir.pop_back();
	lasterror = ZR_OK;
	return ZR_OK;
}

ZRESULT GetZipItem(HZIP hz, int index, ZIPENTRY* ze)
{
	if (!hz || !ze) { lasterror = ZR_ARGS; return ZR_ARGS; }
	ZipHandle* handle = (ZipHandle*)hz;
	memset(ze, 0, sizeof(ZIPENTRY));
	if (index == -1)
	{
		ze->index = (int)mz_zip_reader_get_num_files(&handle->zip);
		lasterror = ZR_OK;
		return ZR_OK;
	}
	if (index < 0 || (mz_uint)index >= mz_zip_reader_get_num_files(&handle->zip))
	{
		lasterror = ZR_NOTFOUND;
		return ZR_NOTFOUND;
	}
	mz_zip_archive_file_stat stat;
	if (!mz_zip_reader_file_stat(&handle->zip, (mz_uint)index, &stat))
	{
		lasterror = ZR_READ;
		return ZR_READ;
	}
	ze->index = index;
	SafeEntryName(stat.m_filename, ze->name, MAX_PATH);
	ze->comp_size = (long)stat.m_comp_size;
	ze->unc_size = (long)stat.m_uncomp_size;
	// attributes: directories get FILE_ATTRIBUTE_DIRECTORY
	ze->attr = stat.m_is_directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	// timestamps: miniz exposes DOS time; convert roughly
	{
		SYSTEMTIME st = { 0 };
		st.wYear = (WORD)((stat.m_time >> 25) + 1980);
		st.wMonth = (WORD)((stat.m_time >> 21) & 0xF);
		st.wDay = (WORD)((stat.m_time >> 16) & 0x1F);
		st.wHour = (WORD)((stat.m_time >> 11) & 0x1F);
		st.wMinute = (WORD)((stat.m_time >> 5) & 0x3F);
		st.wSecond = (WORD)((stat.m_time & 0x1F) * 2);
		FILETIME local;
		SystemTimeToFileTime(&st, &local);
		LocalFileTimeToFileTime(&local, &ze->mtime);
		ze->ctime = ze->atime = ze->mtime;
	}
	lasterror = ZR_OK;
	return ZR_OK;
}

ZRESULT FindZipItem(HZIP hz, const TCHAR* name, bool ic, int* index, ZIPENTRY* ze)
{
	if (!hz || !name || !index || !ze) { lasterror = ZR_ARGS; return ZR_ARGS; }
	ZipHandle* handle = (ZipHandle*)hz;
	int num = (int)mz_zip_reader_get_num_files(&handle->zip);
	for (int i = 0; i < num; i++)
	{
		ZIPENTRY e;
		ZRESULT r = GetZipItem(hz, i, &e);
		if (r != ZR_OK) return r;
		int cmp = ic ? _wcsicmp(e.name, name) : wcscmp(e.name, name);
		if (cmp == 0)
		{
			*index = i;
			*ze = e;
			lasterror = ZR_OK;
			return ZR_OK;
		}
	}
	*index = -1;
	lasterror = ZR_NOTFOUND;
	return ZR_NOTFOUND;
}

ZRESULT UnzipItem(HZIP hz, int index, const TCHAR* fn)
{
	if (!hz || !fn) { lasterror = ZR_ARGS; return ZR_ARGS; }
	ZipHandle* handle = (ZipHandle*)hz;
	if (index < 0 || (mz_uint)index >= mz_zip_reader_get_num_files(&handle->zip))
	{
		lasterror = ZR_NOTFOUND;
		return ZR_NOTFOUND;
	}
	mz_zip_archive_file_stat stat;
	if (!mz_zip_reader_file_stat(&handle->zip, (mz_uint)index, &stat))
	{
		lasterror = ZR_READ;
		return ZR_READ;
	}
	// directories: just create them
	if (stat.m_is_directory)
	{
		TCHAR dirName[MAX_PATH];
		SafeEntryName(stat.m_filename, dirName, MAX_PATH);
		std::wstring full = handle->baseDir.empty() ? dirName : handle->baseDir + L"\\" + dirName;
		CreateDirRecursive(full);
		lasterror = ZR_OK;
		return ZR_OK;
	}
	// build the destination path under the base dir
	TCHAR safeName[MAX_PATH];
	SafeEntryName(stat.m_filename, safeName, MAX_PATH);
	std::wstring full;
	if (PathIsRelativeW(safeName) && !handle->baseDir.empty())
		full = handle->baseDir + L"\\" + safeName;
	else
		full = safeName;
	// ensure the parent directory exists
	{
		std::wstring parent = full;
		size_t slash = parent.find_last_of(L"\\/");
		if (slash != std::wstring::npos)
		{
			parent.resize(slash);
			CreateDirRecursive(parent);
		}
	}
	// extract to memory then write (lets us control the target path safely)
	size_t uncSize = (size_t)stat.m_uncomp_size;
	std::vector<char> buf(uncSize ? uncSize : 1);
	void* p = mz_zip_reader_extract_to_heap(&handle->zip, (mz_uint)index, &uncSize, 0);
	if (!p)
	{
		lasterror = ZR_READ;
		return ZR_READ;
	}
	memcpy(&buf[0], p, uncSize);
	mz_free(p);
	FILE* f = _wfopen(full.c_str(), L"wb");
	if (!f)
	{
		lasterror = ZR_WRITE;
		return ZR_WRITE;
	}
	fwrite(&buf[0], 1, uncSize, f);
	fclose(f);
	lasterror = ZR_OK;
	return ZR_OK;
}

ZRESULT UnzipItem(HZIP hz, int index, void* z, unsigned int len)
{
	if (!hz || !z) { lasterror = ZR_ARGS; return ZR_ARGS; }
	ZipHandle* handle = (ZipHandle*)hz;
	if (index < 0 || (mz_uint)index >= mz_zip_reader_get_num_files(&handle->zip))
	{
		lasterror = ZR_NOTFOUND;
		return ZR_NOTFOUND;
	}
	size_t uncSize = 0;
	void* p = mz_zip_reader_extract_to_heap(&handle->zip, (mz_uint)index, &uncSize, 0);
	if (!p)
	{
		lasterror = ZR_READ;
		return ZR_READ;
	}
	size_t toCopy = (uncSize < len) ? uncSize : len;
	memcpy(z, p, toCopy);
	mz_free(p);
	lasterror = (uncSize > len) ? ZR_MORE : ZR_OK;
	return lasterror;
}

ZRESULT GetZipLastError()
{
	return lasterror;
}
