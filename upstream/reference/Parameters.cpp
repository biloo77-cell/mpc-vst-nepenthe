#include "Parameters.h"

#include <cmath>
#include <numbers>

/*

Parameters is a centralized data store for parameters and values based on parameters. Sample rate is
considered an honorary parameter since it is given at run-time.

*/

const juce::Identifier Parameters::xmlName{"nepenthe"};

Parameters::Parameters(juce::AudioProcessor& processor) :
state(processor, nullptr, xmlName, {
    std::make_unique<juce::AudioParameterFloat>("time", "Time", juce::NormalisableRange<float>(0.5f, 8.0f, 0.01f, 0.7f), 2.2f),
    std::make_unique<juce::AudioParameterInt>("delay", "Delay", 0, maxDelayMs, 30),
    std::make_unique<juce::AudioParameterInt>("bass", "Bass", 0, 100, 50),
    std::make_unique<juce::AudioParameterInt>("treble", "Treble", 0, 100, 50),
    std::make_unique<juce::AudioParameterInt>("width", "Width", 0, 100, 100),
    std::make_unique<juce::AudioParameterInt>("mix", "Mix", 0, 100, 100),
}),
timeState(*state.getRawParameterValue("time")),
delayState(*state.getRawParameterValue("delay")),
bassState(*state.getRawParameterValue("bass")),
trebleState(*state.getRawParameterValue("treble")),
widthState(*state.getRawParameterValue("width")),
mixState(*state.getRawParameterValue("mix")) {}

void Parameters::initialize(const float sampleRate_) {
    sampleRate = sampleRate_;

    spacing = static_cast<int>(std::roundf(sampleRate * loopSeconds / echoes));
    loopLength = spacing * echoes;
    loopCapacity = loopLength * 2;

    delayCapacity = static_cast<int>(0.001f * sampleRate * maxDelayMs) + 1;
    bassFilterK = getFilterK(300.0f);
    mixFilterK = getFilterK(30.0f);
}

void Parameters::update() {
    time = timeState.load();
    feedback = getAmplitude(loopLength);

    delayLength = static_cast<int>(0.001f * sampleRate * delayState.load());

    bassFilterGain = 0.02f * (bassState.load() - 50.0f);
    trebleFilterK = getFilterK(0.01f * trebleState.load() * 2000.0f + 1500.0f);

    width = widthState.load() * 0.005f + 0.5f;
    mix = 0.01f * mixState.load();
}

float Parameters::getFilterK(const float frequency) const {
    constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
    return 1.0f - std::expf(-twoPi * frequency / sampleRate);
}

float Parameters::getAmplitude(const int position) const {
    const float rt60position = sampleRate * time;
    return std::powf(0.001f, position / rt60position);
}
