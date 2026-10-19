#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class ConePressorEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit ConePressorEditor (ConePressorProcessor&);
    ~ConePressorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob { juce::Slider s; juce::Label l; };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& text, const juce::String& id, std::unique_ptr<SA>&);
    void placeKnob (Knob&, juce::Rectangle<int> cell);
    void updateModeVisibility();
    int  getMode() const;

    ConePressorProcessor& proc;
    juce::Image cat;

    Knob inK, outK, mixK, tubeK, laK, fcK;
    juce::ComboBox modeBox, fcTimeBox;
    juce::ToggleButton laLimitBtn { "Limit (statt Compress)" };

    std::unique_ptr<SA> inA, outA, mixA, tubeA, laA, fcA;
    std::unique_ptr<CA> modeA, fcTimeA;
    std::unique_ptr<BA> laLimitA;

    juce::Rectangle<int> catArea, meterArea;
    juce::Colour accent { 0xffe8a33d };
    float grShown = 0.0f;
    int lastMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConePressorEditor)
};
