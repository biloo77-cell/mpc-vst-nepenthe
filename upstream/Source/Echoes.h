#pragma once

#include "Parameters.h"

#include <optional>
#include <random>
#include <vector>

class Echo {
public:
    Echo() = default;
    void updateAmplitude(const Parameters& parameters);
    
    int position{};
    bool sign{};
    float amplitude{};
};

class Echoes {
public:
    Echoes(Parameters& parameters);
    void initialize();
    void updateAmplitudes();

    std::vector<Echo> left;
    std::vector<Echo> right;
private:
    void initializeChannel(std::vector<Echo>& echoes);

    const Parameters& parameters;

    std::mt19937 twister;
    std::optional<float> lastKnownTime;
};
