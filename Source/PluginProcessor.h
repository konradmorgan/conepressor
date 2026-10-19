#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class ConePressorProcessor : public juce::AudioProcessor
{
public:
    ConePressorProcessor();
    ~ConePressorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ConePressor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grMeterDb { 0.0f };   // aktuelle Gain Reduction in dB (positiv)

private:
    double sr = 44100.0;
    float grState  = 0.0f;   // geglättete Gain Reduction (dB)
    float grMemory = 0.0f;   // langsames Gedächtnis für programmabhängige Release-Zeit

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConePressorProcessor)
};
