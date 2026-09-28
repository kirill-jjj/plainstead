// Markup.h: a CMarkup-style navigation wrapper over pugixml (https://pugixml.org).
//
// The rest of the code uses the old CMarkup navigation idiom
// (FindChildElem / IntoElem / OutOfElem / GetChildData); this header keeps that
// API but delegates all actual XML work to pugixml - a spec-compliant,
// fast and crash-safe parser. No hand-written XML parsing is done here.
//

#if !defined(MARKUP_H__948A2705_9E68_11D2_A0BF_00105A27C570_MINI__)
#define MARKUP_H__948A2705_9E68_11D2_A0BF_00105A27C570_MINI__

#include "stdafx.h"
#include "pugixml.hpp"

class CMarkup
{
public:
	CMarkup() = default;
	explicit CMarkup(const std::wstring& szDoc) { SetDoc(szDoc.c_str()); }
	virtual ~CMarkup() = default;

	// Create
	std::wstring GetDoc() const;

	// Navigate
	bool SetDoc(LPCTSTR szDoc);
	bool IsWellFormed() const { return m_wellFormed; }
	bool FindElem(const std::wstring& szName = L"");     // next sibling (optionally by name)
	bool FindChildElem(const std::wstring& szName = L""); // next child (optionally by name)
	bool IntoElem();   // descend into the current child
	bool OutOfElem();  // ascend to the parent
	void ResetPos();
	std::wstring GetTagName() const;
	std::wstring GetChildTagName() const;
	std::wstring GetData() const;
	std::wstring GetChildData() const;
	std::wstring GetError() const { return m_error; }

private:
	static std::wstring WideFromUtf8(const char* s);

	pugi::xml_document m_doc;
	pugi::xml_node m_pos;       // current main position
	pugi::xml_node m_childPos;  // current child position
	bool m_loaded = false;
	bool m_wellFormed = false;
	std::wstring m_error;
};

#endif // !defined(MARKUP_H__MINI__)
