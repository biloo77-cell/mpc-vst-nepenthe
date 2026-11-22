#pragma once

#include "Parameters.h"

#include <array>

class Equalizer {
public:
    Equalizer(Parameters& parameters);
    void initialize();
    float process(float input);
private:
    const Parameters& parameters;

    float bassState{};
    std::array<float, 3> trebleStates{};
};
