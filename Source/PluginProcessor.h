#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include <map>
#include "Compressors.h"

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

    // Undo/Redo (20 Schritte) und A/B-Vergleich
    using Snapshot = std::map<juce::String, float>;
    void switchToSlot (int slot);          // 0 = A, 1 = B
    void copyActiveToOther();              // aktive Einstellung in den anderen Slot kopieren
    int  getActiveSlot() const { return activeSlot.load(); }

    juce::UndoManager undoManager { 1, 20 };   // vor apvts deklarieren!
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grLaDb { 0.0f };   // Gain Reduction LA-2A-Modul (dB)
    std::atomic<float> grFcDb { 0.0f };   // Gain Reduction 670-Modul (dB)

private:
    Snapshot captureSnapshot() const;
    void applySnapshot (const Snapshot&);

    std::array<Snapshot, 2> slots;
    std::atomic<int> activeSlot { 0 };
    juce::CriticalSection slotLock;

    cp::OptoComp  la;
    cp::VarMuComp fc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConePressorProcessor)
};
