#pragma once

#include <JuceHeader.h>

#include "Delay.h"
#include "Echoes.h"
#include "Equalizer.h"
#include "Mixer.h"
#include "Reverberator.h"
#include "Parameters.h"
#include "Timer.h"

#include <vector>

class Processor : public juce::AudioProcessor {
public:
    Processor();

    const juce::String getName() const override { return JucePlugin_Name; }
    bool hasEditor() const override { return true; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layout) const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& block, juce::MidiBuffer&) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    juce::AudioProcessorEditor* createEditor() override;
private:
    Parameters parameters;
    Timer timer;

    Equalizer equalizer;
    Delay delay;
    Echoes echoes;
    Reverberator leftReverb;
    Reverberator rightReverb;
    Mixer mixer;

    std::vector<float> input;
    std::vector<float> leftOutput;
    std::vector<float> rightOutput;
};
