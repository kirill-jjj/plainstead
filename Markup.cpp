// Markup.cpp: implementation of the minimal CMarkup-compatible XML navigator (pure Win32)
//
// Data model: nodes linked as a tree (parent/firstChild/next); content spans
// reference offsets inside m_csDoc. Navigation follows the CMarkup rules used
// by this project: FindElem = next sibling, FindChildElem = next child,
// IntoElem = descend into the current child, OutOfElem = ascend.
//

#include "stdafx.h"
#include "Markup.h"

void CMarkup::Reset()
{
	m_aNodes.clear();
	m_aNodes.push_back(Node()); // node 0 = document root
	m_iPosParent = 0;
	m_iPos = 0;
	m_iPosChild = 0;
	m_bWellFormed = false;
	m_bParsed = false;
	m_csError.clear();
}

void CMarkup::operator=( const CMarkup& markup )
{
	m_csDoc = markup.m_csDoc;
	m_csError = markup.m_csError;
	m_aNodes = markup.m_aNodes;
	m_iPosParent = markup.m_iPosParent;
	m_iPos = markup.m_iPos;
	m_iPosChild = markup.m_iPosChild;
	m_bWellFormed = markup.m_bWellFormed;
	m_bParsed = markup.m_bParsed;
}

// find the '>' closing a tag, honoring quoted attribute values
size_t CMarkup::FindTagEnd(const std::wstring& doc, size_t from)
{
	bool inQuote = false;
	wchar_t quote = 0;
	for (size_t i = from; i < doc.size(); i++)
	{
		wchar_t c = doc[i];
		if (inQuote)
		{
			if (c == quote) inQuote = false;
		}
		else if (c == L'"' || c == L'\'')
		{
			inQuote = true;
			quote = c;
		}
		else if (c == L'>')
		{
			return i;
		}
	}
	return std::wstring::npos;
}

std::wstring CMarkup::DecodeEntities(const std::wstring& s)
{
	std::wstring out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); i++)
	{
		if (s[i] == L'&')
		{
			size_t semi = s.find(L';', i);
			if (semi != std::wstring::npos && semi - i <= 10)
			{
				std::wstring ent = s.substr(i + 1, semi - i - 1);
				if (ent == L"lt") { out += L'<'; i = semi; continue; }
				if (ent == L"gt") { out += L'>'; i = semi; continue; }
				if (ent == L"amp") { out += L'&'; i = semi; continue; }
				if (ent == L"quot") { out += L'"'; i = semi; continue; }
				if (ent == L"apos") { out += L'\''; i = semi; continue; }
				wchar_t* end = NULL;
				if (!ent.empty() && ent[0] == L'#')
				{
					long code = -1;
					if (ent.size() > 1 && (ent[1] == L'x' || ent[1] == L'X'))
						code = wcstol(ent.c_str() + 2, &end, 16);
					else
						code = wcstol(ent.c_str() + 1, &end, 10);
					if (code > 0 && code < 0x10000)
					{
						out += (wchar_t)code;
						i = semi;
						continue;
					}
				}
			}
		}
		out += s[i];
	}
	return out;
}

std::wstring CMarkup::EncodeEntities(const std::wstring& s)
{
	std::wstring out;
	out.reserve(s.size() + 16);
	for (size_t i = 0; i < s.size(); i++)
	{
		switch (s[i])
		{
		case L'<': out += L"&lt;"; break;
		case L'>': out += L"&gt;"; break;
		case L'&': out += L"&amp;"; break;
		case L'"': out += L"&quot;"; break;
		case L'\'': out += L"&apos;"; break;
		default: out += s[i];
		}
	}
	return out;
}

static std::wstring TrimWide(const std::wstring& s)
{
	size_t b = 0, e = s.size();
	while (b < e && iswspace(s[b])) b++;
	while (e > b && iswspace(s[e - 1])) e--;
	return s.substr(b, e - b);
}

bool CMarkup::SetDoc( LPCTSTR szDoc )
{
	std::wstring doc;
	if (szDoc) doc = szDoc;
	Reset();
	m_csDoc = doc;
	m_bParsed = false;
	if (m_csDoc.empty())
	{
		m_bParsed = true;
		m_bWellFormed = false;
		return false;
	}
	int rc = ParseElement(0, 0);
	m_bParsed = true;
	m_bWellFormed = (rc == 0 && m_aNodes[0].firstChild != 0);
	if (rc != 0 && m_csError.empty())
		m_csError = L"parse error";
	// reset the navigation positions, keep the parsed tree
	m_iPosParent = 0;
	m_iPos = 0;
	m_iPosChild = 0;
	return m_bWellFormed;
}

// Parses all elements starting at 'start' as children of 'parent'.
// Returns 0 on success, -1 on error.
int CMarkup::ParseElement(int start, int parent)
{
	int pos = start;
	for (;;)
	{
		if ((size_t)pos >= m_csDoc.size()) return 0;
		size_t lt = m_csDoc.find(L'<', (size_t)pos);
		if (lt == std::wstring::npos) return 0;

		wchar_t c1 = (lt + 1 < m_csDoc.size()) ? m_csDoc[lt + 1] : 0;
		// skip <?...?>, <!--...-->, <![CDATA[...]]>, <!DOCTYPE ...>
		if (c1 == L'?' || c1 == L'!')
		{
			if (m_csDoc.compare(lt, 9, L"<![CDATA[") == 0)
			{
				size_t end = m_csDoc.find(L"]]>", lt + 9);
				if (end == std::wstring::npos) { m_csError = L"Unterminated CDATA"; return -1; }
				pos = (int)(end + 3);
				continue;
			}
			if (m_csDoc.compare(lt, 4, L"<!--") == 0)
			{
				size_t end = m_csDoc.find(L"-->", lt + 4);
				if (end == std::wstring::npos) { m_csError = L"Unterminated comment"; return -1; }
				pos = (int)(end + 3);
				continue;
			}
			if (c1 == L'?')
			{
				size_t end = m_csDoc.find(L"?>", lt + 2);
				if (end == std::wstring::npos) { m_csError = L"Unterminated processing instruction"; return -1; }
				pos = (int)(end + 2);
				continue;
			}
			// DOCTYPE or other <!...>
			size_t end = FindTagEnd(m_csDoc, lt + 2);
			if (end == std::wstring::npos) { m_csError = L"Unterminated declaration"; return -1; }
			pos = (int)(end + 1);
			continue;
		}
		if (c1 == L'/')
			return 0; // a stray end tag at this level: this level is done

		size_t tagEnd = FindTagEnd(m_csDoc, lt);
		if (tagEnd == std::wstring::npos)
		{
			m_csError = L"End of tag not found";
			return -1;
		}

		// extract the tag name
		size_t nameStart = lt + 1;
		while (nameStart < tagEnd && iswspace(m_csDoc[nameStart])) nameStart++;
		size_t nameEnd = nameStart;
		while (nameEnd < tagEnd && !iswspace(m_csDoc[nameEnd]) && m_csDoc[nameEnd] != L'/' && m_csDoc[nameEnd] != L'>') nameEnd++;
		std::wstring name = m_csDoc.substr(nameStart, nameEnd - nameStart);
		if (name.empty())
		{
			m_csError = L"Empty tag name";
			return -1;
		}

		// empty element?
		bool bEmpty = (tagEnd > lt && m_csDoc[tagEnd - 1] == L'/');

		int iThis = (int)m_aNodes.size();
		Node node;
		node.parent = parent;
		node.name = name;
		node.contentStart = node.contentEnd = (tagEnd + 1 <= m_csDoc.size()) ? tagEnd + 1 : m_csDoc.size();
		m_aNodes.push_back(node);

		// link as a sibling
		if (m_aNodes[parent].firstChild == 0)
			m_aNodes[parent].firstChild = iThis;
		else
		{
			int sib = m_aNodes[parent].firstChild;
			while (m_aNodes[sib].next != 0) sib = m_aNodes[sib].next;
			m_aNodes[sib].next = iThis;
		}

		int after;
		if (bEmpty)
		{
			after = (int)tagEnd + 1;
		}
		else
		{
			// find the matching end tag with depth counting
			int depth = 1;
			size_t p = tagEnd + 1;
			size_t closeLt = std::wstring::npos;
			size_t closeGt = std::wstring::npos;
			while (p < m_csDoc.size())
			{
				size_t nlt = m_csDoc.find(L'<', p);
				if (nlt == std::wstring::npos) break;
				if (nlt + 1 < m_csDoc.size() && m_csDoc[nlt + 1] == L'/')
				{
					size_t nEnd = nlt + 2;
					while (nEnd < m_csDoc.size() && iswspace(m_csDoc[nEnd])) nEnd++;
					size_t nEndName = nEnd;
					while (nEndName < m_csDoc.size() && m_csDoc[nEndName] != L'>' && !iswspace(m_csDoc[nEndName])) nEndName++;
					std::wstring closing = m_csDoc.substr(nEnd, nEndName - nEnd);
					if (closing == name && depth == 1)
					{
						closeLt = nlt;
						closeGt = m_csDoc.find(L'>', nlt);
						if (closeGt == std::wstring::npos)
						{
							m_csError = L"Malformed end tag";
							return -1;
						}
						break;
					}
					// an end tag of a nested element: it closes one open child
					if (depth > 1)
						depth--;
					p = (nEndName < m_csDoc.size()) ? nEndName + 1 : m_csDoc.size();
				}
				else
				{
					size_t te = FindTagEnd(m_csDoc, nlt);
					if (te == std::wstring::npos)
					{
						m_csError = L"End of tag not found";
						return -1;
					}
					bool bInnerEmpty = (te > nlt && m_csDoc[te - 1] == L'/');
					wchar_t cIn = (nlt + 1 < m_csDoc.size()) ? m_csDoc[nlt + 1] : 0;
					if (!bInnerEmpty && cIn != L'?' && cIn != L'!')
						depth++;
					p = te + 1;
				}
			}
			if (closeLt == std::wstring::npos)
			{
				m_csError = L"End tag of " + name + L" element not found";
				return -1;
			}
			m_aNodes[iThis].contentStart = tagEnd + 1;
			m_aNodes[iThis].contentEnd = closeLt;
			// parse children within the content
			int rc = ParseElement((int)(tagEnd + 1), iThis);
			if (rc != 0) return rc;
			after = (int)closeGt + 1;
		}
		pos = after;
	}
}

bool CMarkup::FindElem( const std::wstring& szName )
{
	// move to the next sibling of the current main position
	int iPos = m_iPos ? m_aNodes[m_iPos].next : m_aNodes[m_iPosParent].firstChild;
	while (iPos)
	{
		if (szName.empty() || m_aNodes[iPos].name == szName)
		{
			m_iPos = iPos;
			m_iPosParent = m_aNodes[iPos].parent;
			m_iPosChild = 0;
			return true;
		}
		iPos = m_aNodes[iPos].next;
	}
	return false;
}

bool CMarkup::FindChildElem( const std::wstring& szName )
{
	// with no main position, search among the children of the root element
	int iParentNode = m_iPos;
	if (!iParentNode)
	{
		int root = m_aNodes[0].firstChild;
		if (!root) return false;
		iParentNode = root;
	}
	// continue after the current child position
	int iPosChild = m_iPosChild ? m_aNodes[m_iPosChild].next : m_aNodes[iParentNode].firstChild;
	while (iPosChild)
	{
		if (szName.empty() || m_aNodes[iPosChild].name == szName)
		{
			m_iPosChild = iPosChild;
			m_iPosParent = iParentNode;
			return true;
		}
		iPosChild = m_aNodes[iPosChild].next;
	}
	return false;
}

bool CMarkup::IntoElem()
{
	int iInto = 0;
	if (m_iPosChild)
		iInto = m_iPosChild;
	else if (m_iPos && m_aNodes[m_iPos].firstChild)
		iInto = m_aNodes[m_iPos].firstChild;
	if (!iInto)
		return false;
	// descend: the current parent becomes the previous main position
	if (m_iPos)
		m_iPosParent = m_iPos;
	// with no main position (document level), m_iPosParent already points to the root
	m_iPos = iInto;
	m_iPosChild = 0;
	return true;
}

bool CMarkup::OutOfElem()
{
	if (!m_iPos)
		return false;
	// the element we exit becomes the child position of the parent level,
	// so a subsequent FindChildElem continues with the next sibling
	m_iPosChild = m_iPos;
	m_iPos = m_iPosParent;
	m_iPosParent = m_aNodes[m_iPos].parent;
	return true;
}

std::wstring CMarkup::GetTagName() const
{
	if (m_iPos && m_iPos < (int)m_aNodes.size())
		return m_aNodes[m_iPos].name;
	return std::wstring();
}

std::wstring CMarkup::GetChildTagName() const
{
	if (m_iPosChild && m_iPosChild < (int)m_aNodes.size())
		return m_aNodes[m_iPosChild].name;
	return std::wstring();
}

std::wstring CMarkup::GetData() const
{
	if (!m_iPos || m_iPos >= (int)m_aNodes.size()) return std::wstring();
	const Node& n = m_aNodes[m_iPos];
	if (n.firstChild) return std::wstring(); // container element, no text data
	if (n.contentStart <= n.contentEnd && n.contentEnd <= m_csDoc.size())
		return TrimWide(DecodeEntities(m_csDoc.substr(n.contentStart, n.contentEnd - n.contentStart)));
	return std::wstring();
}

std::wstring CMarkup::GetChildData() const
{
	if (!m_iPosChild || m_iPosChild >= (int)m_aNodes.size()) return std::wstring();
	const Node& n = m_aNodes[m_iPosChild];
	if (n.firstChild) return std::wstring(); // container element, no text data
	if (n.contentStart <= n.contentEnd && n.contentEnd <= m_csDoc.size())
		return TrimWide(DecodeEntities(m_csDoc.substr(n.contentStart, n.contentEnd - n.contentStart)));
	return std::wstring();
}

bool CMarkup::AddElem( const std::wstring& szName, const std::wstring& szData )
{
	std::wstring ins = L"<" + szName + L">" + EncodeEntities(szData) + L"</" + szName + L">\r\n";
	return SetDoc((m_csDoc + ins).c_str());
}

bool CMarkup::AddChildElem( const std::wstring& szName, const std::wstring& szData )
{
	// simplified: appends a child element to the document body
	return AddElem(szName, szData);
}
