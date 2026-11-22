#pragma once

#include <JuceHeader.h>

#include <atomic>

class Parameters {
public:
    Parameters(juce::AudioProcessor& processor);

    void initialize(float sampleRate);
    void update();
    float getFilterK(float frequency) const;
    float getAmplitude(int position) const;

    static constexpr int echoes = 1000;
    static constexpr float loopSeconds = 1.0f;
    static constexpr float gain = 90.0f / echoes;
    static constexpr int maxDelayMs = 200;

    float sampleRate{};

    int spacing{};
    int loopLength{};
    int loopCapacity{};

    float time{};
    float feedback{};

    int delayCapacity{};
    int delayLength{};

    float bassFilterGain{};
    float bassFilterK{};
    float trebleFilterK{};

    float width{};
    float mix{};
    float mixFilterK{};

    static const juce::Identifier xmlName;
    juce::AudioProcessorValueTreeState state;
private:
    const std::atomic<float>& timeState;
    const std::atomic<float>& delayState;
    const std::atomic<float>& bassState;
    const std::atomic<float>& trebleState;
    const std::atomic<float>& widthState;
    const std::atomic<float>& mixState;
};
