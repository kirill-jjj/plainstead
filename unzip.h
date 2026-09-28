// unzip.h: zip reading API backed by miniz (https://github.com/richgel999/miniz).
// The API intentionally matches the classic "Zip Utils" surface used by this
// project (OpenZip / GetZipItem / UnzipItem / SetUnzipBaseDir / CloseZip) so
// the callers do not change.
//

#ifndef _UNZIP_H
#define _UNZIP_H

#include "stdafx.h"

typedef void* HZIP;                     // handle to an opened zip archive
typedef unsigned long ZRESULT;          // result code (ZR_*)

#define ZIP_HANDLE   1
#define ZIP_FILENAME 2
#define ZIP_MEMORY   3

#define ZR_OK         0x00000000
#define ZR_RECENT     0x00000001
#define ZR_GENMASK    0x0000FF00
#define ZR_NODUPH     0x00000100
#define ZR_NOFILE     0x00000200
#define ZR_NOALLOC    0x00000300
#define ZR_WRITE      0x00000400
#define ZR_NOTFOUND   0x00000500
#define ZR_MORE       0x00000600
#define ZR_CORRUPT    0x00000700
#define ZR_READ       0x00000800
#define ZR_PASSWORD   0x00000900
#define ZR_ARGS       0x00000A00
#define ZR_PARTIALUNZ 0x00000B00
#define ZR_NOTMMAP    0x00000C00
#define ZR_MEMSIZE    0x00000D00
#define ZR_FAILED     0x00000E00
#define ZR_ENCRYPTED  0x00000F00
#define ZR_NODATA     0x00001000
#define ZR_NOENCRYPT  0x00001100
#define ZR_SEEK       0x00001200
#define ZR_NOCOMPRESS 0x00001300
#define ZR_FLATE      0x00001400

struct ZIPENTRY
{
	int index;              // index of the entry
	TCHAR name[MAX_PATH];   // name within the archive (no absolute paths)
	WORD attr;              // file attributes
	FILETIME ctime, atime, mtime;
	long comp_size;         // sizes of item, compressed and uncompressed
	long unc_size;
};

// open an archive from a file name, a handle, or a memory block
HZIP OpenZip(const TCHAR* fn, const char* password);
HZIP OpenZip(void* z, unsigned int len, const char* password);
HZIP OpenZipHandle(HANDLE h, const char* password);

// item information: index -1 returns the total number of items in ze->index
ZRESULT GetZipItem(HZIP hz, int index, ZIPENTRY* ze);
// find an item by name
ZRESULT FindZipItem(HZIP hz, const TCHAR* name, bool ic, int* index, ZIPENTRY* ze);
// unzip one item to a file name
ZRESULT UnzipItem(HZIP hz, int index, const TCHAR* fn);
// unzip one item into memory (call repeatedly while it returns ZR_MORE)
ZRESULT UnzipItem(HZIP hz, int index, void* z, unsigned int len);
// set the base directory for relative paths in UnzipItem
ZRESULT SetUnzipBaseDir(HZIP hz, const TCHAR* dir);
// close the archive
ZRESULT CloseZip(HZIP hz);
// last error of the most recent zip call
ZRESULT GetZipLastError();

#endif // _UNZIP_H
