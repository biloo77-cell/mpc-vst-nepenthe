/* Upstream's Source/Parameters.cpp without JUCE: initialize(), update(), getFilterK() and getAmplitude() are upstream's,
 * line for line; update() reads `values` (set by src/engine.cpp) where upstream loads its atomics.
 * SPDX-License-Identifier: GPL-3.0-only */
#include "Parameters.h"

#include <cmath>

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
    time = values.time;
    feedback = getAmplitude(loopLength);

    delayLength = static_cast<int>(0.001f * sampleRate * values.delay);

    bassFilterGain = 0.02f * (values.bass - 50.0f);
    trebleFilterK = getFilterK(0.01f * values.treble * 2000.0f + 1500.0f);

    width = values.width * 0.005f + 0.5f;
    mix = 0.01f * values.mix;
}

float Parameters::getFilterK(const float frequency) const {
    constexpr float twoPi = 2.0f * 3.14159265358979323846f;
    return 1.0f - std::exp(-twoPi * frequency / sampleRate);
}

float Parameters::getAmplitude(const int position) const {
    const float rt60position = sampleRate * time;
    return std::pow(0.001f, position / rt60position);
}
