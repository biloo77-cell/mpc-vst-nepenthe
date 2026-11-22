#include "LookAndFeel.h"
#include "Fonts.h"

const juce::Font sansFont = Fonts::getSans();
const juce::Font monoFont = Fonts::getMono();

juce::Font LookAndFeel::getLabelFont(juce::Label& label) {
    return label.isEditable() ? monoFont : sansFont;
}

juce::Label* LookAndFeel::createSliderTextBox(juce::Slider& slider) {
    auto label = new juce::Label();

    label->setJustificationType(juce::Justification::centred);
    label->setKeyboardType(juce::TextInputTarget::decimalKeyboard);

    label->setColour(juce::Label::textColourId, juce::Colours::white);
    label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    label->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    label->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    label->setColour(juce::TextEditor::highlightColourId, slider.findColour(juce::Slider::thumbColourId));
    label->setColour(juce::CaretComponent::caretColourId, juce::Colours::white);

    return label;
}

void LookAndFeel::drawLinearSlider(juce::Graphics& graphics,
                                   const int x, const int y, const int width, const int height,
                                   const float sliderPos, float, float,
                                   juce::Slider::SliderStyle, juce::Slider& slider) {
    constexpr float thumbSize = 14.0f;

    const float bothX = static_cast<float>(x) + 0.5f * width;
    const float startY = static_cast<float>(y);
    const float endY = static_cast<float>(y + height);

    const juce::Point start(bothX, startY);
    const juce::Point end(bothX, endY);
    const juce::Point mid(bothX, sliderPos);

    juce::Path topToMid;
    topToMid.startNewSubPath(start);
    topToMid.lineTo(mid);

    juce::Path midToBottom;
    midToBottom.startNewSubPath(mid);
    midToBottom.lineTo(end);

    const juce::Colour thumbColor = slider.findColour(juce::Slider::thumbColourId);
    const juce::Colour trackColor(0xff181d40);

    const juce::ColourGradient lowerGradient(thumbColor.withAlpha(0.4f), bothX, sliderPos + 0.5f * thumbSize, thumbColor.withAlpha(0.0f), bothX, endY, false);
    const juce::ColourGradient upperGradient(trackColor, bothX, startY, trackColor.withAlpha(0.0f), bothX, sliderPos - 0.5f * thumbSize, false);
    const juce::PathStrokeType strokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    graphics.setGradientFill(upperGradient);
    graphics.strokePath(topToMid, strokeType);

    graphics.setGradientFill(lowerGradient);
    graphics.strokePath(midToBottom, strokeType);

    graphics.setColour(thumbColor);
    graphics.fillRoundedRectangle(0.5f * width - 0.5f * thumbSize, sliderPos - 0.5f * thumbSize, thumbSize, thumbSize, 3.0f);
}

void LookAndFeel::fillTextEditorBackground(juce::Graphics& graphics, int, int, juce::TextEditor& editor) {
    graphics.fillAll(editor.findColour(juce::TextEditor::backgroundColourId));
    editor.setJustification(juce::Justification::centred);
}
