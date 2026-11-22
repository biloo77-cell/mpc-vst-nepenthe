#pragma once

#include "Parameters.h"

#include <optional>

class Mixer {
public:
    Mixer(Parameters& parameters);
    void initialize();
    void advanceSample();

    float stereoSelf;
    float stereoCross;
    float mixWet;
    float mixDry;
private:
    const Parameters& parameters;

    std::optional<float> widthState;
    std::optional<float> mixState;
};
