# WavetableSynthProject
A polyphonic wavetable synth with proper mixing, envelope release threshold, resampling factors, and fixed volume scaling.

To compile from the Visual Studio command prompt:
cl /EHsc /Iinclude WavetableSynthDriver.cpp WavetableSynth.cpp AudioData.lib portaudio_x86.lib portmidi.lib ADSR.lib Resample.lib MidiIn.lib Wave.lib
