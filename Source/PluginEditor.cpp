#include "PluginEditor.h"
#include "BinaryData.h"

ConePressorEditor::ConePressorEditor (ConePressorProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    cat = juce::ImageCache::getFromMemory (BinaryData::cat_png, BinaryData::cat_pngSize);

    setupKnob (inK,   "Input",          "in",       inA);
    setupKnob (outK,  "Output",         "out",      outA);
    setupKnob (mixK,  "Mix",            "mix",      mixA);
    setupKnob (tubeK, "Tube",           "tube",     tubeA);
    setupKnob (laK,   "Peak Reduction", "laPeak",   laA);
    setupKnob (fcK,   "Threshold",      "fcThresh", fcA);

    modeBox.addItemList (proc.apvts.getParameter ("mode")->getAllValueStrings(), 1);
    addAndMakeVisible (modeBox);
    modeA = std::make_unique<CA> (proc.apvts, "mode", modeBox);

    fcTimeBox.addItemList (proc.apvts.getParameter ("fcTime")->getAllValueStrings(), 1);
    addAndMakeVisible (fcTimeBox);
    fcTimeA = std::make_unique<CA> (proc.apvts, "fcTime", fcTimeBox);

    addAndMakeVisible (laLimitBtn);
    laLimitA = std::make_unique<BA> (proc.apvts, "laLimit", laLimitBtn);

    setSize (760, 460);
    updateModeVisibility();
    startTimerHz (30);
}

ConePressorEditor::~ConePressorEditor()
{
    stopTimer();
}

int ConePressorEditor::getMode() const
{
    return (int) proc.apvts.getRawParameterValue ("mode")->load();
}

void ConePressorEditor::setupKnob (Knob& k, const juce::String& text, const juce::String& id,
                                   std::unique_ptr<SA>& att)
{
    k.s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    k.l.setText (text, juce::dontSendNotification);
    k.l.setJustificationType (juce::Justification::centred);
    k.l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    addAndMakeVisible (k.s);
    addAndMakeVisible (k.l);
    att = std::make_unique<SA> (proc.apvts, id, k.s);
}

void ConePressorEditor::placeKnob (Knob& k, juce::Rectangle<int> cell)
{
    k.l.setBounds (cell.removeFromTop (18));
    k.s.setBounds (cell);
}

void ConePressorEditor::updateModeVisibility()
{
    const int m = getMode();
    lastMode = m;
    const bool la = (m == 0);

    laK.s.setVisible (la);  laK.l.setVisible (la);  laLimitBtn.setVisible (la);
    fcK.s.setVisible (!la); fcK.l.setVisible (!la); fcTimeBox.setVisible (!la);

    accent = la ? juce::Colour (0xffe8a33d) : juce::Colour (0xff4fc3b0);
    for (auto* k : { &inK, &outK, &mixK, &tubeK, &laK, &fcK })
    {
        k->s.setColour (juce::Slider::rotarySliderFillColourId, accent);
        k->s.setColour (juce::Slider::thumbColourId, accent);
    }
    repaint();
}

void ConePressorEditor::timerCallback()
{
    if (getMode() != lastMode)
        updateModeVisibility();

    const float gr = proc.grMeterDb.load();
    grShown = std::max (gr, grShown * 0.85f);
    repaint (catArea.getUnion (meterArea).expanded (20));
}

void ConePressorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);

    auto top = r.removeFromTop (40);
    modeBox.setBounds (top.removeFromRight (180).reduced (0, 6));
    r.removeFromTop (8);

    auto left  = r.removeFromLeft (190);
    auto right = r.removeFromRight (190);

    const int rowH = left.getHeight() / 3;

    // linke Spalte: Input, modusabhängiger Hauptregler, modusabhängige Extras
    placeKnob (inK, left.removeFromTop (rowH));
    auto specific = left.removeFromTop (rowH);
    placeKnob (laK, specific);
    placeKnob (fcK, specific);
    auto extra = left.reduced (10, 30);
    laLimitBtn.setBounds (extra.withHeight (28));
    fcTimeBox.setBounds (extra.withHeight (28));

    // rechte Spalte: Output, Mix, Tube
    placeKnob (outK,  right.removeFromTop (rowH));
    placeKnob (mixK,  right.removeFromTop (rowH));
    placeKnob (tubeK, right);

    // Mitte: Katze + GR-Meter
    meterArea = r.removeFromBottom (44).reduced (10, 8);
    catArea   = r.reduced (8);
}

void ConePressorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient grad (juce::Colour (0xff2a2e36), 0.0f, 0.0f,
                               juce::Colour (0xff101216), 0.0f, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();

    g.setColour (accent);
    g.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    g.drawText ("CONE-PRESSOR", 20, 14, 300, 34, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("Opto-Compressor / Variable-Mu  |  Cone-of-Shame Technology (TM)",
                190, 14, 380, 34, juce::Justification::centredLeft);

    // Der Gag: Die Katze "pumpt" mit der Gain Reduction (je mehr GR, desto kleiner).
    if (cat.isValid())
    {
        const float scale = 1.0f - juce::jlimit (0.0f, 0.18f, grShown * 0.012f);
        auto area = catArea.toFloat();
        auto drawRect = area.withSizeKeepingCentre (area.getWidth() * scale, area.getHeight() * scale);

        juce::Path p;
        p.addRoundedRectangle (drawRect, 20.0f);

        g.saveState();
        g.reduceClipRegion (p);
        g.drawImage (cat, drawRect,
                     juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        g.restoreState();

        g.setColour (accent.withAlpha (0.8f));
        g.strokePath (p, juce::PathStrokeType (2.0f));
    }

    // GR-Meter
    auto m = meterArea.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (m, 6.0f);
    const float frac = juce::jlimit (0.0f, 1.0f, grShown / 20.0f);
    g.setColour (accent);
    g.fillRoundedRectangle (m.withWidth (m.getWidth() * frac), 6.0f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText ("GR  -" + juce::String (grShown, 1) + " dB", meterArea, juce::Justification::centred);
}
