#pragma once

#include "Parameters.h"

#include <vector>

class Delay {
public:
    Delay(Parameters& parameters);
    void initialize();
    void updateLength();
    float process(float input);
private:
    const Parameters& parameters;

    std::vector<float> data;
    int readPosition{};
    int writePosition{};
};
