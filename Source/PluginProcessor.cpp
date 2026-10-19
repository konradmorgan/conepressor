#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    inline float dbToLin (float db) { return std::pow (10.0f, db * 0.05f); }

    // Soft-Knee Gain Computer (Standard-Formel), liefert Gain Reduction in dB (>= 0)
    float softKneeGR (float levelDb, float threshDb, float ratio, float kneeDb)
    {
        const float over = levelDb - threshDb;
        const float slope = 1.0f - 1.0f / ratio;
        if (2.0f * over < -kneeDb) return 0.0f;
        if (2.0f * std::abs (over) <= kneeDb)
        {
            const float t = over + kneeDb * 0.5f;
            return slope * t * t / (2.0f * kneeDb);
        }
        return slope * over;
    }

    // Variable-Mu-Charakteristik (Fairchild-Stil): Ratio steigt mit dem Pegel über der Schwelle
    float fairchildGR (float levelDb, float threshDb)
    {
        const float over = levelDb - threshDb;
        if (over <= 0.0f) return 0.0f;
        const float rMax = 8.0f;
        const float w = 15.0f;
        return (1.0f - 1.0f / rMax) * over * over / (over + w);
    }

    // Weiche Röhren-Sättigung mit leichtem Bias (gerade Harmonische), Kleinsignalverstärkung ~ 1
    inline float saturate (float x, float amt)
    {
        if (amt <= 0.0f) return x;
        const float drive = 1.0f + 3.0f * amt;
        const float bias  = 0.08f * amt;
        return (std::tanh (drive * x + bias) - std::tanh (bias)) / drive;
    }
}

ConePressorProcessor::ConePressorProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ConePressorProcessor::createLayout()
{
    using Attr = juce::AudioParameterFloatAttributes;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "LA-2A", "Fairchild 670" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "in", 1 }, "Input",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, Attr().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "out", 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, Attr().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, Attr().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tube", 1 }, "Tube",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 20.0f, Attr().withLabel ("%")));

    // LA-2A
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "laPeak", 1 }, "Peak Reduction",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "laLimit", 1 }, "Limit", false));

    // Fairchild
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fcThresh", 1 }, "Threshold",
        juce::NormalisableRange<float> (-40.0f, 0.0f, 0.1f), -18.0f, Attr().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "fcTime", 1 }, "Time Constant",
        juce::StringArray { "1", "2", "3", "4", "5", "6" }, 1));

    return layout;
}

void ConePressorProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    grState = 0.0f;
    grMemory = 0.0f;
}

bool ConePressorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void ConePressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);
    if (nCh == 0) return;

    const int   mode     = (int) apvts.getRawParameterValue ("mode")->load();
    const float inG      = dbToLin (apvts.getRawParameterValue ("in")->load());
    const float outG     = dbToLin (apvts.getRawParameterValue ("out")->load());
    const float mix      = apvts.getRawParameterValue ("mix")->load() * 0.01f;
    const float tube     = apvts.getRawParameterValue ("tube")->load() * 0.01f;
    const float laPeak   = apvts.getRawParameterValue ("laPeak")->load();
    const bool  laLimit  = apvts.getRawParameterValue ("laLimit")->load() > 0.5f;
    const float fcThresh = apvts.getRawParameterValue ("fcThresh")->load();
    const int   fcTime   = (int) apvts.getRawParameterValue ("fcTime")->load();

    auto coef = [this] (float ms) { return std::exp (-1.0f / (float) (sr * (double) ms * 0.001)); };

    // LA-2A: Peak Reduction verschiebt die Schwelle, Compress/Limit ändert Ratio + Knie
    const float laThresh = -4.0f - laPeak * 0.4f;
    const float laRatio  = laLimit ? 12.0f : 3.0f;
    const float laKnee   = laLimit ? 6.0f  : 10.0f;
    const float laAtk    = coef (10.0f);

    // Fairchild-Zeitkonstanten 1-6 (Attack ms, Release-Bereich ms, programmabhängig ja/nein)
    static const float fcAtkMs[6]  = { 0.2f, 0.2f, 0.2f, 0.4f, 0.8f, 0.4f };
    static const float fcRelLo[6]  = { 300.0f, 800.0f, 2000.0f, 5000.0f, 2000.0f, 300.0f };
    static const float fcRelHi[6]  = { 300.0f, 800.0f, 2000.0f, 5000.0f, 10000.0f, 25000.0f };
    const int   tc     = juce::jlimit (0, 5, fcTime);
    const float fcAtk  = coef (fcAtkMs[tc]);

    const float memCoef = 1.0f - coef (3000.0f);

    float* data[2] = { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float maxGr = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { 0.0f, 0.0f };
        float peak = 0.0f;
        for (int c = 0; c < nCh; ++c)
        {
            x[c] = data[c][i] * inG;
            peak = std::max (peak, std::abs (x[c]));          // gelinkter Detektor
        }

        const float lvlDb  = 20.0f * std::log10 (std::max (peak, 1.0e-6f));
        const float target = (mode == 0) ? softKneeGR (lvlDb, laThresh, laRatio, laKnee)
                                         : fairchildGR (lvlDb, fcThresh);

        grMemory += (target - grMemory) * memCoef;

        if (target > grState)
        {
            const float a = (mode == 0) ? laAtk : fcAtk;
            grState = a * grState + (1.0f - a) * target;
        }
        else
        {
            float relMs;
            if (mode == 0)
                relMs = 60.0f + (2500.0f - 60.0f) * juce::jlimit (0.0f, 1.0f, grMemory / 10.0f);
            else
                relMs = fcRelLo[tc] + (fcRelHi[tc] - fcRelLo[tc]) * juce::jlimit (0.0f, 1.0f, grMemory / 8.0f);

            const float r = coef (relMs);
            grState = r * grState + (1.0f - r) * target;
        }

        const float g = dbToLin (-grState);
        for (int c = 0; c < nCh; ++c)
        {
            const float wet = saturate (x[c] * g, tube);
            data[c][i] = (x[c] * (1.0f - mix) + wet * mix) * outG;
        }

        maxGr = std::max (maxGr, grState);
    }

    grMeterDb.store (maxGr);
}

juce::AudioProcessorEditor* ConePressorProcessor::createEditor()
{
    return new ConePressorEditor (*this);
}

void ConePressorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ConePressorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ConePressorProcessor();
}
