#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AQWAXAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AQWAXAudioProcessorEditor (AQWAXAudioProcessor&);
    ~AQWAXAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AQWAXAudioProcessor& processor;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    Knob water, aero, gloss, space, width, mix, output;

    void setupKnob(Knob&, const juce::String&, const juce::String&);
    void drawKnob(juce::Graphics&, Knob&, juce::Rectangle<int>);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AQWAXAudioProcessorEditor)
};
