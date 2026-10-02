#include "PluginEditor.h"

AQWAXAudioProcessorEditor::AQWAXAudioProcessorEditor (AQWAXAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize(560, 430);

    setupKnob(water, "WATER", "Water");
    setupKnob(aero, "AERO", "Aero");
    setupKnob(gloss, "GLOSS", "Gloss");
    setupKnob(space, "SPACE", "Space");
    setupKnob(width, "WIDTH", "Width");
    setupKnob(mix, "MIX", "Mix");
    setupKnob(output, "OUTPUT", "Output");
}

void AQWAXAudioProcessorEditor::setupKnob(Knob& k, const juce::String& id, const juce::String& name)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    k.slider.setRange(id == "OUTPUT" ? -12.0 : 0.0,
                      id == "OUTPUT" ? 6.0 : 1.0,
                      id == "OUTPUT" ? 0.01 : 0.001);
    k.slider.setValue(processor.getAPVTS().getRawParameterValue(id)->load());
    k.slider.setDoubleClickReturnValue(true, id == "OUTPUT" ? 0.0 : 0.2);
    addAndMakeVisible(k.slider);

    k.label.setText(name, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(juce::Font(13.0f, juce::Font::bold));
    k.label.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), id, k.slider);
}

void AQWAXAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    juce::Colour top(0xffd9fbff);
    juce::Colour bottom(0xff67cfe8);
    g.setGradientFill(juce::ColourGradient(top, 0, 0, bottom, 0, getHeight(), false));
    g.fillAll();

    // Glossy glass panel.
    auto panel = b.reduced(18.0f);
    g.setColour(juce::Colours::white.withAlpha(0.24f));
    g.fillRoundedRectangle(panel, 24.0f);

    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.drawRoundedRectangle(panel, 24.0f, 1.5f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(30.0f, juce::Font::bold));
    g.drawFittedText("AQWAX", 25, 18, getWidth() - 50, 42,
                     juce::Justification::centred, 1);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(11.0f));
    g.drawFittedText("LIQUID AUDIO • AERO PROCESSING", 25, 56,
                     getWidth() - 50, 20, juce::Justification::centred, 1);

    // Decorative bubbles.
    for (int i = 0; i < 7; ++i)
    {
        const float x = 30.0f + i * 83.0f;
        const float y = 85.0f + (i % 3) * 22.0f;
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.fillEllipse(x, y, 12.0f + (i % 3) * 5.0f, 12.0f + (i % 3) * 5.0f);
    }
}

void AQWAXAudioProcessorEditor::drawKnob(juce::Graphics&, Knob&, juce::Rectangle<int>)
{
}

void AQWAXAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(35);
    area.removeFromTop(92);

    const int gap = 10;
    const int cellW = (area.getWidth() - gap * 3) / 4;
    const int cellH = 135;

    std::array<Knob*, 7> knobs { &water, &aero, &gloss, &space, &width, &mix, &output };

    for (int i = 0; i < 7; ++i)
    {
        const int row = i / 4;
        const int col = i % 4;
        auto cell = juce::Rectangle<int>(
            area.getX() + col * (cellW + gap),
            area.getY() + row * (cellH + 8),
            cellW, cellH);

        knobs[i]->slider.setBounds(cell.reduced(3));
        knobs[i]->label.setBounds(cell.getX(), cell.getBottom() - 25,
                                   cell.getWidth(), 22);
    }
}
