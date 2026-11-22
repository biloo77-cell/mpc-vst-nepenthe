#include "Mixer.h"

/*

Mixer manages width and mix coefficients, with low-pass smoothing.

*/

Mixer::Mixer(Parameters& parameters) :
parameters(parameters) {}

void Mixer::initialize() {
    // Don't initialize with the parameter values, because they may change before processing.
    widthState.reset();
    mixState.reset();
}

void Mixer::advanceSample() {
    if (!widthState.has_value()) widthState = parameters.width;
    if (!mixState.has_value()) mixState = parameters.mix;

    *widthState += parameters.mixFilterK * (parameters.width - *widthState);
    stereoSelf = *widthState;
    stereoCross = 1.0f - stereoSelf;

    *mixState += parameters.mixFilterK * (parameters.mix - *mixState);
    mixWet = *mixState;
    mixDry = 1.0f - mixWet;
}
