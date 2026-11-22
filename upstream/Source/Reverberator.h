#pragma once

#include "Echoes.h"
#include "Parameters.h"

#include <vector>

class Reverberator {
public:
    Reverberator(Parameters& parameters, std::vector<Echo>& echoes);
    void initialize();
    void process(const std::vector<float>& input, std::vector<float>& output);
private:
    const Parameters& parameters;
    const std::vector<Echo>& echoes;

    std::vector<float> data;
    int readPosition{};
    int feedbackPosition{};
};
