#include "Equalizer.h"

/*

Equalizer implements the Bass and Treble parameters.

Bass is a low-pass filter set at a fixed frequency, blended in parallel, with polarity flipped for
values less than 50.

Treble is a stack of three low-pass filters with a variable frequency.

*/

Equalizer::Equalizer(Parameters& parameters) :
parameters(parameters) {}

void Equalizer::initialize() {
    bassState = 0.0f;
    trebleStates.fill(0.0f);
}

float Equalizer::process(float value) {
    bassState += parameters.bassFilterK * (value - bassState);
    value += bassState * parameters.bassFilterGain;

    for (float& state : trebleStates) {
        state += parameters.trebleFilterK * (value - state);
        value = state;
    }

    return value;
}
