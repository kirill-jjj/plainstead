#include "Wave.h"
#include <iostream>
#include <fstream>

int Wave::currVol = 100;

void Wave::SetVolume(int vol)
{
	if (vol < 0) currVol = 0;
	else if (vol>100) currVol = 100;
	else currVol = vol;
}

int Wave::GetVolume()
{
	return currVol;
}

Wave::Wave(char *filename)
{
	sample = BASS_SampleLoad(FALSE, filename, 0, 0, 1, 0);
}

void Wave::play(bool ignore_settings)
{
	if (sample) {
		HCHANNEL chan = BASS_SampleGetChannel(sample, FALSE);
		if (chan) {
			if (!ignore_settings) BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, GetVolume()/100.0f);
			else BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, 0.8f);
			BASS_ChannelPlay(sample, FALSE);
		}
	}
}

bool Wave::isok()
{
	return (sample);
}

Wave::~Wave()
{
	if (sample) BASS_SampleFree(sample);
}
