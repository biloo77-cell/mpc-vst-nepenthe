#include "Processor.h"
#include "Editor.h"

/*

Processor is the entry point from the host program into Nepenthe. It is responsible for data flow
and state management.

Nepenthe has a mono-in, stereo-out design. There is an independent mono reverb algorithm working on
each channel. The high-level processing flow goes like this:

1. Collapse the input samples into mono.
2. Run the mono input through pre-processing: EQ and delay.
3. Run the pre-processed mono input through the left and right reverbs.
4. Run the stereo output through post-processing: width and dry/wet.

*/

Processor::Processor() :
AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
parameters(*this),
equalizer(parameters),
delay(parameters),
echoes(parameters),
leftReverb(parameters, echoes.left),
rightReverb(parameters, echoes.right),
mixer(parameters) {}

bool Processor::isBusesLayoutSupported(const BusesLayout& layout) const {
    const int inputs = layout.getMainInputChannels();
    const int outputs = layout.getMainOutputChannels();

    return outputs == 2 && (inputs == 2 || inputs == 1);
}

void Processor::prepareToPlay(const double sampleRate, const int samplesPerBlock) {
    input.reserve(samplesPerBlock);
    leftOutput.reserve(samplesPerBlock);
    rightOutput.reserve(samplesPerBlock);

    parameters.initialize(static_cast<float>(sampleRate));

    equalizer.initialize();
    delay.initialize();
    echoes.initialize();
    leftReverb.initialize();
    rightReverb.initialize();
    mixer.initialize();

    timer.reset();
}

void Processor::processBlock(juce::AudioBuffer<float>& block, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    if (isRealtime() == Realtime::yes && timer.boop()) {
        equalizer.initialize();
        delay.initialize();
        leftReverb.initialize();
        rightReverb.initialize();
        mixer.initialize();
    }

    parameters.update();
    delay.updateLength();
    echoes.updateAmplitudes();

    const int blockSamples = block.getNumSamples();
    const bool isStereo = getMainBusNumInputChannels() == 2;

    input.resize(blockSamples);
    leftOutput.resize(blockSamples);
    rightOutput.resize(blockSamples);

    const float* readLeft = block.getReadPointer(0);
    const float* readRight = block.getReadPointer(isStereo);
    float* writeLeft = block.getWritePointer(0);
    float* writeRight = block.getWritePointer(1);

    for (int sample = 0; sample < blockSamples; sample++) {
        const float left = readLeft[sample];
        const float right = readRight[sample];
        float mono = (left + right) * 0.5f;
        mono = equalizer.process(mono);
        mono = delay.process(mono);
        input[sample] = mono;
    }

    leftReverb.process(input, leftOutput);
    rightReverb.process(input, rightOutput);

    for (int sample = 0; sample < blockSamples; sample++) {
        mixer.advanceSample();

        float left = leftOutput[sample];
        float right = rightOutput[sample];

        left = mixer.stereoSelf * left + mixer.stereoCross * right;
        right = mixer.stereoSelf * right + mixer.stereoCross * left;

        left = mixer.mixWet * left + mixer.mixDry * readLeft[sample];
        right = mixer.mixWet * right + mixer.mixDry * readRight[sample];

        writeLeft[sample] = left;
        writeRight[sample] = right;
    }
}

void Processor::getStateInformation(juce::MemoryBlock& destData) {
    std::unique_ptr<juce::XmlElement> xml = parameters.state.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void Processor::setStateInformation(const void* data, const int size) {
    std::unique_ptr<juce::XmlElement> xml = getXmlFromBinary(data, size);
    if (xml.get() == nullptr) return;
    if (!xml->hasTagName(parameters.xmlName)) return;
    parameters.state.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* Processor::createEditor() {
    return new Editor(*this, parameters.state);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new Processor();
}
