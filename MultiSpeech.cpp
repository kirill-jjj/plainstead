// MultiSpeech.cpp: implementation of the MultiSpeech class
//

#include "stdafx.h"
#include "MultiSpeech.h"
#include "Tolk.h"

MultiSpeech::MultiSpeech()
{
	Tolk_Load();
}

void MultiSpeech::Say(const std::wstring& text)
{
	// do not speak empty text
	if (Tolk_HasSpeech())
	{
		if (!Tolk_Output(text.c_str())) {
			OutputDebugStringW(L"Failed to output text\n");
		}
	}
}

std::wstring MultiSpeech::GetCurrentReader()
{
	const wchar_t* reader = Tolk_DetectScreenReader();
	std::wstring res(reader ? reader : L"");
	return res;
}

void MultiSpeech::Unload()
{
	Tolk_Unload();
}

MultiSpeech::~MultiSpeech()
{
}
