#include "Editor.h"

const juce::Colour timeColor(0xffc13791);
const juce::Colour eqColor(0xff6b42ba);
const juce::Colour mixColor(0xff2953c9);

constexpr int sliderHeight = 230;
constexpr int sliderWidth = 60;
constexpr int topMargin = 45;
constexpr int bottomMargin = 25;
constexpr int leftRightMargin = 30;

Editor::Editor(juce::AudioProcessor& processor, juce::AudioProcessorValueTreeState& state) :
AudioProcessorEditor(&processor),
timeAttach(state, "time", timeSlider),
delayAttach(state, "delay", delaySlider),
bassAttach(state, "bass", bassSlider),
trebleAttach(state, "treble", trebleSlider),
widthAttach(state, "width", widthSlider),
mixAttach(state, "mix", mixSlider) {
    initialize(0, "Time", timeSlider, timeLabel, timeColor);
    initialize(1, "Delay", delaySlider, delayLabel, timeColor);
    initialize(2, "Bass", bassSlider, bassLabel, eqColor);
    initialize(3, "Treble", trebleSlider, trebleLabel, eqColor);
    initialize(4, "Width", widthSlider, widthLabel, mixColor);
    initialize(5, "Mix", mixSlider, mixLabel, mixColor);

    setSize(leftRightMargin * 2 + sliderWidth * 6, topMargin + bottomMargin + sliderHeight);
}

void Editor::initialize(const int offset, const juce::String& labelText, juce::Slider& slider, juce::Label& label, const juce::Colour color) {
    slider.setLookAndFeel(&look);
    slider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    slider.setBounds(leftRightMargin + sliderWidth * offset, topMargin, sliderWidth, sliderHeight);
    slider.setColour(juce::Slider::thumbColourId, color);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.attachToComponent(&slider, false);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

void Editor::paint(juce::Graphics& graphics) {
    graphics.fillAll(juce::Colour(0xff060710));
}
