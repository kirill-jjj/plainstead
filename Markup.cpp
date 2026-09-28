// Markup.cpp: CMarkup-style navigation over pugixml (pure Win32 project)
//

#include "stdafx.h"
#include "Markup.h"

std::wstring CMarkup::WideFromUtf8(const char* s)
{
	if (!s || !*s) return std::wstring();
	int need = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	if (need <= 1) return std::wstring();
	std::wstring out(need - 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], need);
	return out;
}

// small writer that collects the serialized document into a std::string
struct StringWriter : pugi::xml_writer
{
	std::string out;
	void write(const void* data, size_t size) override { out.append(static_cast<const char*>(data), size); }
};

std::wstring CMarkup::GetDoc() const
{
	// re-serialize the document (rarely used; kept for API compatibility)
	StringWriter w;
	const_cast<pugi::xml_document&>(m_doc).save(w, "", pugi::format_raw);
	return WideFromUtf8(w.out.c_str());
}

bool CMarkup::SetDoc(LPCTSTR szDoc)
{
	m_error.clear();
	m_pos = pugi::xml_node();
	m_childPos = pugi::xml_node();
	m_loaded = false;
	m_wellFormed = false;
	if (!szDoc || !*szDoc) return false;

	std::string utf8;
	{
		int need = WideCharToMultiByte(CP_UTF8, 0, szDoc, -1, NULL, 0, NULL, NULL);
		if (need > 1)
		{
			utf8.resize(need - 1);
			WideCharToMultiByte(CP_UTF8, 0, szDoc, -1, &utf8[0], need, NULL, NULL);
		}
	}

	// pugixml parses robustly: it never crashes on malformed input and
	// reports what it could recover instead
	pugi::xml_parse_result result = m_doc.load_string(utf8.c_str(), pugi::parse_default | pugi::parse_fragment);
	m_loaded = true;
	m_wellFormed = result.status == pugi::status_ok;
	if (!m_wellFormed)
		m_error = WideFromUtf8(result.description());
	return m_wellFormed;
}

bool CMarkup::FindElem(const std::wstring& szName)
{
	if (!m_loaded) return false;
	// next sibling of the current position; from the document root when at start
	pugi::xml_node start = m_pos ? m_pos.next_sibling() : m_doc.first_child();
	for (pugi::xml_node n = start; n; n = n.next_sibling())
	{
		if (n.type() != pugi::node_element) continue;
		if (!szName.empty() && WideFromUtf8(n.name()) != szName) continue;
		m_pos = n;
		m_childPos = pugi::xml_node();
		return true;
	}
	return false;
}

bool CMarkup::FindChildElem(const std::wstring& szName)
{
	if (!m_loaded) return false;
	// with no main position, search among children of the root element
	pugi::xml_node parent = m_pos ? m_pos : m_doc.first_child();
	if (!parent) return false;
	pugi::xml_node start = m_childPos ? m_childPos.next_sibling() : parent.first_child();
	for (pugi::xml_node n = start; n; n = n.next_sibling())
	{
		if (n.type() != pugi::node_element) continue;
		if (!szName.empty() && WideFromUtf8(n.name()) != szName) continue;
		m_childPos = n;
		return true;
	}
	return false;
}

bool CMarkup::IntoElem()
{
	pugi::xml_node target = m_childPos ? m_childPos : m_pos;
	if (!target) return false;
	m_pos = target;
	m_childPos = pugi::xml_node();
	return true;
}

bool CMarkup::OutOfElem()
{
	if (!m_pos || !m_pos.parent()) return false;
	// the element we exit becomes the child position of the parent level,
	// so a subsequent FindChildElem continues with the next sibling
	m_childPos = m_pos;
	m_pos = m_pos.parent();
	// stepping out of the root element back to the document is allowed
	return true;
}

void CMarkup::ResetPos()
{
	m_pos = pugi::xml_node();
	m_childPos = pugi::xml_node();
}

std::wstring CMarkup::GetTagName() const
{
	return m_pos ? WideFromUtf8(m_pos.name()) : std::wstring();
}

std::wstring CMarkup::GetChildTagName() const
{
	return m_childPos ? WideFromUtf8(m_childPos.name()) : std::wstring();
}

std::wstring CMarkup::GetData() const
{
	return m_pos ? WideFromUtf8(m_pos.text().as_string()) : std::wstring();
}

std::wstring CMarkup::GetChildData() const
{
	return m_childPos ? WideFromUtf8(m_childPos.text().as_string()) : std::wstring();
}
