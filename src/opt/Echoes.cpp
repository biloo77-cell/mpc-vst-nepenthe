/* MPC PORT: replaces upstream/Source/Echoes.cpp in the build (header unchanged). One change, marked below: the
 * Mersenne Twister gets a fixed seed instead of std::random_device, so every instance and every project load draws the
 * same echo pattern (same distribution as upstream; reproducible bounces; the Open MPC host test needs two instances
 * to match). User decision 2026-11-22.
 * SPDX-License-Identifier: GPL-3.0-only */
#include "Echoes.h"

/*

Echoes is a container for randomly-generated echoes used in Reverberator.cpp.

The echo generation is based on the concept of velvet noise, described in:

  Välimäki, Vesa, Heidi-Maria Lehtonen, and Marko Takanen. "A perceptual study on velvet noise and
  its variants at different pulse densities." IEEE Transactions on Audio, Speech, and Language
  Processing 21.7 (2013): 1481-1488.

  Välimäki, Vesa, et al. "Late reverberation synthesis using filtered velvet noise." Applied
  Sciences 7.5 (2017): 483.

Each side of the stereo reverb has its own set of randomly-generated echo positions and signs.

*/

Echoes::Echoes(Parameters& parameters) :
parameters(parameters),
twister(0x4e657065u) {}   // MPC PORT: fixed seed (upstream: std::random_device()())

void Echoes::initialize() {
    left.resize(parameters.echoes);
    right.resize(parameters.echoes);

    initializeChannel(left);
    initializeChannel(right);

    updateAmplitudes();
}

void Echoes::initializeChannel(std::vector<Echo>& echoes) {
    std::uniform_int_distribution<int> positions{1, parameters.spacing};
    std::uniform_int_distribution<int> signs{0, 1};

    int offset = 0;

    for (Echo& echo : echoes) {
        echo.position = offset + positions(twister);
        echo.sign = signs(twister);
        offset += parameters.spacing;
    }
}

void Echoes::updateAmplitudes() {
    if (parameters.time != lastKnownTime) {
        lastKnownTime = parameters.time;

        for (Echo& echo : left) echo.updateAmplitude(parameters);
        for (Echo& echo : right) echo.updateAmplitude(parameters);
    }
}

void Echo::updateAmplitude(const Parameters& parameters) {
    amplitude = parameters.getAmplitude(position) * (sign ? 1 : -1);
}
