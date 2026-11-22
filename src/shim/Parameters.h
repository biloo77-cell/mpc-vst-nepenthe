#pragma once
/* Stands in for upstream's Source/Parameters.h (upstream/reference/) without JUCE: the same fields, constants and
 * formulas; only the parameter store changes. Upstream reads six atomics from its AudioProcessorValueTreeState; here
 * the engine writes the six values, in upstream's units, into `values` before each update().
 * SPDX-License-Identifier: GPL-3.0-only */

class Parameters {
public:
    Parameters() = default;

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

    /* Upstream's parameter values: time 0.5..8 s, delay 0..200 ms, bass/treble/width/mix 0..100. */
    struct {
        float time = 2.2f, delay = 30, bass = 50, treble = 50, width = 100, mix = 100;
    } values;
};
