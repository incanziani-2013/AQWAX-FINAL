#include "PluginProcessor.h"
#include "PluginEditor.h"

AQWAXAudioProcessor::AQWAXAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AQWAX_STATE", createParameterLayout())
#else
    : apvts (*this, nullptr, "AQWAX_STATE", createParameterLayout())
#endif
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AQWAXAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto pct = [](const juce::String& id, const juce::String& name, float def)
    {
        return std::make_unique<juce::AudioParameterFloat>(
            id, name, juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), def);
    };

    p.push_back(pct("WATER",  "Water",  0.25f));
    p.push_back(pct("AERO",   "Aero",   0.30f));
    p.push_back(pct("GLOSS",  "Gloss",  0.20f));
    p.push_back(pct("SPACE",  "Space",  0.20f));
    p.push_back(pct("WIDTH",  "Width",  0.35f));
    p.push_back(pct("MIX",    "Mix",    0.18f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "OUTPUT", "Output", juce::NormalisableRange<float>(-12.0f, 6.0f, 0.01f), 0.0f));

    return { p.begin(), p.end() };
}

void AQWAXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    lowCut.reset();
    highShelfFilter.reset();
    compressor.reset();
    reverb.reset();

    lowCut.prepare(spec);
    highShelfFilter.prepare(spec);
    compressor.prepare(spec);
    reverb.prepare(spec);

    lowCut.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    highShelfFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    wetBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    delayBuffer.setSize(getTotalNumOutputChannels(), static_cast<int>(sampleRate * 0.25) + samplesPerBlock + 4);
    delayBuffer.clear();
    delayWritePosition = 0;
}

void AQWAXAudioProcessor::releaseResources()
{
    wetBuffer.setSize(0, 0);
    delayBuffer.setSize(0, 0);
}

bool AQWAXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void AQWAXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float water  = apvts.getRawParameterValue("WATER")->load();
    const float aero   = apvts.getRawParameterValue("AERO")->load();
    const float gloss  = apvts.getRawParameterValue("GLOSS")->load();
    const float space  = apvts.getRawParameterValue("SPACE")->load();
    const float width  = apvts.getRawParameterValue("WIDTH")->load();
    const float mix    = apvts.getRawParameterValue("MIX")->load();
    const float outputDb = apvts.getRawParameterValue("OUTPUT")->load();

    wetBuffer.makeCopyOf(buffer, true);

    // WATER: gentle low-pass + resonant liquid smoothing.
    const float waterCutoff = 18000.0f - water * 9000.0f;
    lowCut.setCutoffFrequency(juce::jmax(1000.0f, waterCutoff));
    lowCut.setResonance(0.15f + water * 0.35f);

    juce::dsp::AudioBlock<float> block(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    lowCut.process(ctx);

    // AERO + GLOSS: high-frequency lift using a simple one-pole shelf-like blend.
    const float glossAmount = gloss * 0.18f;
    const float aeroAmount  = aero * 0.10f;

    for (int ch = 0; ch < wetBuffer.getNumChannels(); ++ch)
    {
        auto* data = wetBuffer.getWritePointer(ch);
        float previous = 0.0f;

        for (int i = 0; i < wetBuffer.getNumSamples(); ++i)
        {
            const float x = data[i];
            const float high = x - previous;
            previous += 0.12f * (x - previous);
            data[i] = x + high * (glossAmount + aeroAmount);
        }
    }

    // WATER: short stereo delay / reflection.
    const int delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 1,
                                          static_cast<int>(currentSampleRate * 0.055));
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* src = wetBuffer.getReadPointer(ch);
        auto* db  = delayBuffer.getWritePointer(ch);
        auto* out = wetBuffer.getWritePointer(ch);

        for (int i = 0; i < wetBuffer.getNumSamples(); ++i)
        {
            const int readPos = (delayWritePosition + i - delaySamples
                                 + delayBuffer.getNumSamples()) % delayBuffer.getNumSamples();
            const float delayed = db[readPos];
            db[(delayWritePosition + i) % delayBuffer.getNumSamples()] =
                src[i] + delayed * 0.12f;
            out[i] = src[i] + delayed * (0.12f + water * 0.22f);
        }
    }
    delayWritePosition = (delayWritePosition + wetBuffer.getNumSamples())
                         % delayBuffer.getNumSamples();

    // SPACE: compact, clean reverb.
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.18f + space * 0.40f;
    rp.damping = 0.65f;
    rp.wetLevel = space * 0.22f;
    rp.dryLevel = 0.0f;
    rp.width = 0.65f + width * 0.30f;
    rp.freezeMode = 0.0f;
    reverb.setParameters(rp);

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> wetCtx(wetBlock);
    reverb.process(wetCtx);

    // Gentle glue.
    compressor.setThreshold(-10.0f - gloss * 4.0f);
    compressor.setRatio(1.2f + gloss * 1.0f);
    compressor.setAttack(25.0f);
    compressor.setRelease(100.0f);

    juce::dsp::ProcessContextReplacing<float> compCtx(wetBlock);
    compressor.process(compCtx);

    // Stereo width.
    if (wetBuffer.getNumChannels() >= 2)
    {
        auto* L = wetBuffer.getWritePointer(0);
        auto* R = wetBuffer.getWritePointer(1);
        const float w = 1.0f + width * 0.45f;

        for (int i = 0; i < wetBuffer.getNumSamples(); ++i)
        {
            const float mid = 0.5f * (L[i] + R[i]);
            const float side = 0.5f * (L[i] - R[i]) * w;
            L[i] = mid + side;
            R[i] = mid - side;
        }
    }

    // Dry/wet + output.
    const float wetAmount = juce::jlimit(0.0f, 1.0f, mix);
    const float gain = juce::Decibels::decibelsToGain(outputDb);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* dry = buffer.getWritePointer(ch);
        auto* wet = wetBuffer.getReadPointer(juce::jmin(ch, wetBuffer.getNumChannels() - 1));

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            dry[i] = (dry[i] * (1.0f - wetAmount) + wet[i] * wetAmount) * gain;
    }
}

juce::AudioProcessorEditor* AQWAXAudioProcessor::createEditor()
{
    return new AQWAXAudioProcessorEditor(*this);
}

void AQWAXAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
}

void AQWAXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
