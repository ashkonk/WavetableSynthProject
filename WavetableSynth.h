//------------------------------------------------------------------------------
//
// File Name:	WavetableSynth.h
// Author:	    Ashkon Khalkhali (ashkon.khalkhali)
// Assignment:	Assignment 9
// Course:		CS245-S25
//
// Copyright © 2025 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#ifndef CS245_WAVETABLESYNTH_H
#define CS245_WAVETABLESYNTH_H

#include "MidiIn.h"
#include "AudioData.h"
#include "Resample.h"
#include "ADSR.h"


class WavetableSynth : private MidiIn {
public:
	// Constructor: Initialize with MIDI device number and sampling rate
	WavetableSynth(int devno, int R);

	// Destructor
	~WavetableSynth(void);

	// Calculate the current output sample
	float output(void);

	// Advance to the next sample
	void next(void);

private:
	// Struct to hold note data
	struct Voice {
		int midiNote;
		float velocity;
		bool active;
		int channel;
		std::unique_ptr<ADSR> env;
		std::unique_ptr<Resample> resampler;

		Voice() : midiNote(-1), velocity(0.0f), active(false), channel(0),
			env(nullptr), resampler(nullptr) {}
	};

	// Pitch bend (per channel)
	std::vector<float> pitchBends;

	// Sampling rate of the synthesizer output
	int sampleRate;

	// The sample data used for playback (WavetableSynth.wav)
	std::unique_ptr<AudioData> sampleData;

	// Array of voices for polyphony
	std::vector<Voice> voices;
	// Current pitch bend value (-1.0 to 1.0)
	float pitchBend;

	// Loop start and end indices
	unsigned loopStart;
	unsigned loopEnd;

	//////////////////////
	// Helper Functions
	//////////////////////

	// Convert MIDI note number to frequency in Hz
	float noteToFreq(int note, float bend = 0.0f);

	// Calculate the resampling factor for a given MIDI note
	float calculateResampleFactor(int midiNote, float pitchBendCents = 0.0f);

	// Start playing a new note
	//void startNote(int midiNote, float velocity);

	int WavetableSynth::findAvailableVoice(int midiNote);

	// Release the currently playing note
	void releaseNote(int midiNote, int channel);

	//////////////////////
	// MIDI event handlers
	//////////////////////

	void onNoteOn(int channel, int note, int velocity) override;
	void onNoteOff(int channel, int note) override;
	void onPitchWheelChange(int channel, float value) override;
};

#endif

