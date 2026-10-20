#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Hardware-Look: schwarze Knoepfe mit Skala, Kipphebel, Kontrolllampen
class HardwareLook : public juce::LookAndFeel_V4
{
public:
    explicit HardwareLook (bool fairchildStyle) : fc (fairchildStyle) {}

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    juce::Colour textCol() const
    {
        return fc ? juce::Colours::white.withAlpha (0.92f) : juce::Colour (0xff15171a);
    }

private:
    bool fc;
};

// Analoges Zeigerinstrument fuer die Gain Reduction (0-20 dB)
class VuMeter : public juce::Component
{
public:
    explicit VuMeter (juce::Colour face) : faceColour (face) {}
    void setValue (float db);
    void paint (juce::Graphics&) override;

private:
    juce::Colour faceColour;
    float value = 0.0f;
};

class ConePressorEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit ConePressorEditor (ConePressorProcessor&);
    ~ConePressorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override { grabKeyboardFocus(); }

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob { juce::Slider s; juce::Label l; };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& text, const juce::String& id, std::unique_ptr<SA>&,
                    HardwareLook&, const juce::String& scaleLabels, juce::Colour textColour);
    void setupToggle (juce::ToggleButton&, const juce::String& id, std::unique_ptr<BA>&,
                      HardwareLook&, const juce::String& kind, const juce::String& textOff = {},
                      const juce::String& textOn = {});
    void placeKnob (Knob&, juce::Rectangle<int> cell);
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, bool fairchildStyle);

    // LookAndFeels muessen vor den Komponenten deklariert sein (werden danach zerstoert)
    HardwareLook laLook { false };
    HardwareLook fcLook { true };

    ConePressorProcessor& proc;
    juce::Image cat;

    VuMeter laMeter { juce::Colour (0xfff0e2b0) };
    VuMeter fcMeter { juce::Colour (0xffe9e4cf) };

    Knob laPeakK, laGainK;
    Knob fcInK, fcThreshK, fcTimeK, fcOutK;
    Knob inK, outK, mixK, tubeK;

    juce::ToggleButton laOnBtn { "IN CIRCUIT" }, fcOnBtn { "IN CIRCUIT" };
    juce::ToggleButton laLimitBtn, orderBtn;
    juce::TextButton undoBtn { "UNDO" }, redoBtn { "REDO" }, abA { "A" }, abB { "B" }, copyBtn { "A > B" };

    std::unique_ptr<SA> laPeakA, laGainA, fcInA, fcThreshA, fcTimeA, fcOutA, inA, outA, mixA, tubeA;
    std::unique_ptr<BA> laOnA, fcOnA, laLimitA, orderA;

    juce::Rectangle<int> laPanel, fcPanel, centerPanel, catArea, captionArea;
    float laShown = 0.0f, fcShown = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConePressorEditor)
};
