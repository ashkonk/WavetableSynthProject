//------------------------------------------------------------------------------
//
// File Name:    WavetableSynth.cpp
// Author:       Ashkon Khalkhali (ashkon.khalkhali)
// Course:       CS245-S25
//
// Implements a polyphonic wavetable synth with proper mixing,
// envelope release threshold, correct resampling factors,
// and fixed volume scaling.
// Copyright © 2025 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#define _USE_MATH_DEFINES

#include "WavetableSynth.h"
#include <cmath>
#include <iostream>

static const int MAX_VOICES = 10;

WavetableSynth::WavetableSynth(int devno, int R)
    : MidiIn(devno), sampleRate(R)
{
    voices.resize(MAX_VOICES);
    pitchBends.assign(16, 0.0f);

    // Load sample and set loop points
    sampleData = std::make_unique<AudioData>("WavetableSynth.wav");
    loopStart = sampleData->frames() / 4;
    loopEnd = sampleData->frames() - 1;

    start();
}

WavetableSynth::~WavetableSynth(void)
{
    stop();
}

float WavetableSynth::output(void)
{
    float mixedOutput = 0.0f;
    int activeVoiceCount = 0;

    // Sum contributions of each active voice
    for (Voice& voice : voices) {
        if (voice.active && voice.resampler && voice.env) {
            float sampleValue = voice.resampler->output();
            mixedOutput += sampleValue * voice.env->output() * voice.velocity;
            ++activeVoiceCount;
        }
    }

    // Apply constant mix factor to avoid volume jumps
    if (activeVoiceCount > 1) {
        mixedOutput *= 0.3f;
    }

    return mixedOutput;
}

void WavetableSynth::next(void)
{
    const float EFFECTIVE_ZERO = std::pow(10.0f, -96.0f / 20.0f);

    for (Voice& voice : voices) {
        if (!voice.active) continue;
        if (voice.resampler && voice.env) {
            voice.resampler->next();
            voice.env->next();

            // Disable voice when envelope falls below audible threshold
            if (voice.env->mode() == ADSR::RELEASE && voice.env->output() < EFFECTIVE_ZERO) {
                voice.active = false;
            }
        }
    }
}

float WavetableSynth::noteToFreq(int note, float bend)
{
    float semitones = (note - 69) + (bend * 2.0f);
    return 440.0f * std::pow(2.0f, semitones / 12.0f);
}

float WavetableSynth::calculateResampleFactor(int midiNote, float pitchBendCents)
{
    // Base frequency of the sample (220 Hz sine wave)
    const float baseFreq = 220.0f;
    float targetFreq = noteToFreq(midiNote, 0.0f);
    float ratio = targetFreq / baseFreq;

    // Adjust for output vs recorded sample rates
    const float recordedSampleRate = 44100.0f;
    ratio *= (recordedSampleRate / static_cast<float>(sampleRate));

    // Apply pitch bend in cents
    if (pitchBendCents != 0.0f) {
        ratio *= std::pow(2.0f, pitchBendCents / 1200.0f);
    }

    return ratio;
}

void WavetableSynth::releaseNote(int midiNote, int channel)
{
    for (Voice& voice : voices) {
        if (voice.active && voice.midiNote == midiNote && voice.channel == channel) {
            if (voice.env) voice.env->sustainOff();
        }
    }
}

int WavetableSynth::findAvailableVoice(int midiNote)
{
    // Retrigger existing note
    for (size_t i = 0; i < voices.size(); ++i) {
        if (voices[i].active && voices[i].midiNote == midiNote)
            return static_cast<int>(i);
    }
    // Find free voice
    for (size_t i = 0; i < voices.size(); ++i) {
        if (!voices[i].active)
            return static_cast<int>(i);
    }
    // Steal oldest release voice
    int best = -1;
    float minEnv = 1.0f;
    for (size_t i = 0; i < voices.size(); ++i) {
        if (voices[i].env && voices[i].env->mode() == ADSR::RELEASE) {
            float envv = voices[i].env->output();
            if (best < 0 || envv < minEnv) {
                best = static_cast<int>(i);
                minEnv = envv;
            }
        }
    }
    return (best >= 0 ? best : 0);
}

void WavetableSynth::onNoteOn(int channel, int note, int velocity)
{
    if (velocity == 0) {
        onNoteOff(channel, note);
        return;
    }

    int idx = findAvailableVoice(note);
    Voice& voice = voices[idx];

    float bendCents = pitchBends[channel] * 200.0f;
    float factor = calculateResampleFactor(note, bendCents);

    // Use longer release for audible decay
    voice.env = std::make_unique<ADSR>(0.05f, 0.3f, 0.7f, 2.0f, static_cast<float>(sampleRate));
    voice.resampler = std::make_unique<Resample>(sampleData.get(), 0, factor, loopStart, loopEnd);

    voice.midiNote = note;
    voice.velocity = velocity / 127.0f;
    voice.active = true;
    voice.channel = channel;

    voice.env->reset();
}

void WavetableSynth::onNoteOff(int channel, int note)
{
    releaseNote(note, channel);
}

void WavetableSynth::onPitchWheelChange(int channel, float value)
{
    pitchBends[channel] = value;
    float cents = value * 200.0f;
    for (Voice& voice : voices) {
        if (voice.active && voice.channel == channel && voice.resampler) {
            voice.resampler->pitchOffset(cents);
        }
    }
}
