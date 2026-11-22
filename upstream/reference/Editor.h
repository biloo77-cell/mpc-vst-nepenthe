#pragma once

#include <JuceHeader.h>

#include "LookAndFeel.h"

class Editor : public juce::AudioProcessorEditor {
public:
    Editor(juce::AudioProcessor& processor, juce::AudioProcessorValueTreeState& parameterState);

    void paint(juce::Graphics&) override;
private:
    void initialize(int offset, const juce::String& labelText, juce::Slider& slider, juce::Label& label, juce::Colour color);

    juce::Slider timeSlider;
    juce::Label timeLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment timeAttach;

    juce::Slider delaySlider;
    juce::Label delayLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment delayAttach;

    juce::Slider bassSlider;
    juce::Label bassLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment bassAttach;

    juce::Slider trebleSlider;
    juce::Label trebleLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment trebleAttach;

    juce::Slider widthSlider;
    juce::Label widthLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment widthAttach;

    juce::Slider mixSlider;
    juce::Label mixLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttach;

    LookAndFeel look;
};
