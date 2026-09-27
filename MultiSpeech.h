// MultiSpeech.h: Tolk-based speech output singleton
//

#pragma once

#include "stdafx.h"

class MultiSpeech
{
public:
	static MultiSpeech& getInstance() {
		static MultiSpeech instance;
		return instance;
	}
	void Say(const std::wstring& text); // speak the text
	std::wstring GetCurrentReader();
	void Unload();
private:
	MultiSpeech();
	MultiSpeech(const MultiSpeech&);
	MultiSpeech& operator=(const MultiSpeech&);
	~MultiSpeech();

};
