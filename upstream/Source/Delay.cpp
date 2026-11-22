#include "Delay.h"

/*

Delay is a simple variable-length delay, used as the pre-delay.

*/

Delay::Delay(Parameters& parameters) :
parameters(parameters) {}

void Delay::initialize() {
    data.assign(parameters.delayCapacity, 0.0f);
}

void Delay::updateLength() {
    readPosition = writePosition - parameters.delayLength;
    if (readPosition < 0) readPosition += data.size();
}

float Delay::process(const float input) {
    data[writePosition] = input;
    const float output = data[readPosition];

    if (++readPosition == data.size()) readPosition = 0;
    if (++writePosition == data.size()) writePosition = 0;

    return output;
}
