// Markup.h: a minimal XML navigation class with the CMarkup-style API.
//
// Replaces CMarkup Release 6.5 Lite (which was MFC/CString based) with a
// small self-contained std::wstring implementation. It supports the
// navigation subset used by this project:
//   SetDoc / FindElem / FindChildElem / IntoElem / OutOfElem / ResetPos /
//   GetChildData / GetData / GetTagName / GetChildTagName / IsWellFormed /
//   AddElem / AddChildElem
// Entities (&amp; &lt; &gt; &quot; &apos; &#NN; &#xNN;) are decoded in GetData.
//

#if !defined(MARKUP_H__948A2705_9E68_11D2_A0BF_00105A27C570_MINI__)
#define MARKUP_H__948A2705_9E68_11D2_A0BF_00105A27C570_MINI__

#include "stdafx.h"

class CMarkup
{
public:
	CMarkup() { Reset(); };
	CMarkup( const std::wstring& szDoc ) { Reset(); SetDoc( szDoc.c_str() ); };
	CMarkup( const CMarkup& markup ) { *this = markup; };
	void operator=( const CMarkup& markup );
	virtual ~CMarkup() {};

	// Create
	std::wstring GetDoc() const { return m_csDoc; };
	bool AddElem( const std::wstring& szName, const std::wstring& szData = L"" );
	bool AddChildElem( const std::wstring& szName, const std::wstring& szData = L"" );

	// Navigate
	bool SetDoc( LPCTSTR szDoc );
	bool IsWellFormed() { return m_bWellFormed; };
	bool FindElem( const std::wstring& szName = L"" );    // next sibling (optionally by name)
	bool FindChildElem( const std::wstring& szName = L"" ); // next child (optionally by name)
	bool IntoElem();   // descend into the current element
	bool OutOfElem();  // ascend to the parent
	void ResetChildPos() { m_iPosChild = 0; };
	void ResetMainPos() { m_iPos = m_iPosParent; m_iPosChild = 0; };
	void ResetPos() { m_iPosParent = 0; m_iPos = 0; m_iPosChild = 0; };
	std::wstring GetTagName() const;
	std::wstring GetChildTagName() const;
	std::wstring GetData() const;
	std::wstring GetChildData() const;
	std::wstring GetError() const { return m_csError; };

protected:
	void Reset();
	int ParseElement(int start, int parent); // returns index of the next free node or -1
	static std::wstring DecodeEntities(const std::wstring& s);
	static std::wstring EncodeEntities(const std::wstring& s);
	static size_t FindTagEnd(const std::wstring& doc, size_t from); // position of '>' honoring quotes

	struct Node
	{
		int parent;
		int firstChild;
		int next;      // next sibling
		size_t nameStart;  // tag name offsets inside m_csDoc
		size_t nameLen;
		size_t contentStart; // inner content offsets
		size_t contentEnd;
		std::wstring name;
		std::wstring data;   // decoded text content (for leaf nodes)
		Node() : parent(0), firstChild(0), next(0), nameStart(0), nameLen(0), contentStart(0), contentEnd(0) {}
	};

	std::wstring m_csDoc;
	std::wstring m_csError;
	std::vector<Node> m_aNodes; // node 0 is the document root
	int m_iPosParent;
	int m_iPos;
	int m_iPosChild;
	bool m_bWellFormed;
	bool m_bParsed;
};

#endif // !defined(MARKUP_H__MINI__)
