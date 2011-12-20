//
// MyAudio.h
// stkExample
//
// Created by Jorge Herrera on 2011-10-26.
//
// Class that encapsulates RtAudio, mainly to simplify the main file

#ifndef __MY_AUDIO_H__
#define __MY_AUDIO_H__

#include "Common.h"
#include "stk/Stk.h"
#include "stk/RtAudio.h"

/**
* Wrapper class to set up RtAudio
*/
class MyAudio {

  public:
    MyAudio( unsigned int nChann = 2, float sr = MY_SRATE,
             unsigned int buffSize = 512, RtAudioFormat = RTAUDIO_FLOAT64 );
    ~MyAudio();

  public:
    void setup( RtAudioCallback callback, void * userData = NULL );
    void start();
    void stop();
    unsigned int bufferSize() { return m_bufferSize; };

  private:
    RtAudio * m_audio;
    unsigned int m_numChannels;
    float m_sampleRate;
    unsigned int m_bufferSize;
    RtAudioFormat m_format;
};

#endif
