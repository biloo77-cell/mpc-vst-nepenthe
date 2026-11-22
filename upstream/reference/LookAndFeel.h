#pragma once

#include <JuceHeader.h>

class LookAndFeel : public juce::LookAndFeel_V4 {
public:
    juce::Font getLabelFont(juce::Label& label) override;

    juce::Label* createSliderTextBox(juce::Slider& slider) override;
    
    void drawLinearSlider(juce::Graphics& graphics, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void fillTextEditorBackground(juce::Graphics& graphics, int width, int height, juce::TextEditor& editor) override;
};
