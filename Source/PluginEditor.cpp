#include "PluginEditor.h"
#include "BinaryData.h"

using juce::Colour;
namespace Colours = juce::Colours;
using juce::Point;
using juce::Rectangle;

static juce::Font mkFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
void HardwareLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                     float pos, float startA, float endA, juce::Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size  = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto c      = bounds.getCentre();
    const float rOuter = size * 0.5f;
    const float rKnob  = rOuter * 0.50f;
    const float angle  = startA + pos * (endA - startA);

    auto pt = [&] (float r, float a) { return Point<float> (c.x + r * std::sin (a), c.y - r * std::cos (a)); };

    // Skala (Ticks + Zahlen) auf dem Panel
    juce::StringArray labels;
    labels.addTokens (s.getProperties()["labels"].toString(), ",", "");
    const int n = labels.size();
    if (n > 1)
    {
        g.setFont (mkFont (10.0f, true));
        for (int i = 0; i < n; ++i)
        {
            const float a = startA + (float) i / (float) (n - 1) * (endA - startA);
            g.setColour (textCol());
            g.drawLine (juce::Line<float> (pt (rKnob * 1.28f, a), pt (rKnob * 1.46f, a)), 1.6f);
            g.drawText (labels[i], Rectangle<float> (30.0f, 12.0f).withCentre (pt (rKnob * 1.72f, a)),
                        juce::Justification::centred, false);

            if (i < n - 1)
            {
                const float a2 = startA + ((float) i + 0.5f) / (float) (n - 1) * (endA - startA);
                g.setColour (textCol().withAlpha (0.7f));
                g.drawLine (juce::Line<float> (pt (rKnob * 1.28f, a2), pt (rKnob * 1.38f, a2)), 1.0f);
            }
        }
    }

    // Schatten
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (Rectangle<float> (rKnob * 2.3f, rKnob * 2.3f).withCentre (c.translated (2.0f, 3.5f)));

    // 670: Knopf mit hellem, gerilltem Rand (Skirt)
    if (fc)
    {
        const float rs = rKnob * 1.14f;
        juce::ColourGradient sg (Colour (0xffb4b8bd), c.x - rs * 0.5f, c.y - rs * 0.6f,
                                 Colour (0xff5b5f64), c.x + rs * 0.6f, c.y + rs * 0.7f, true);
        g.setGradientFill (sg);
        g.fillEllipse (Rectangle<float> (rs * 2.0f, rs * 2.0f).withCentre (c));
        g.setColour (Colours::black.withAlpha (0.45f));
        for (int i = 0; i < 36; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 36.0f;
            g.drawLine (juce::Line<float> (pt (rs * 0.86f, a), pt (rs, a)), 1.2f);
        }
    }
    else
    {
        // LA-2A: dezenter Chrom-Ring
        juce::ColourGradient rg (Colour (0xffe8eaec), c.x - rKnob, c.y - rKnob,
                                 Colour (0xff7d8186), c.x + rKnob, c.y + rKnob, false);
        g.setGradientFill (rg);
        g.fillEllipse (Rectangle<float> (rKnob * 2.12f, rKnob * 2.12f).withCentre (c));
    }

    // Knopfkoerper (schwarz, leicht glaenzend)
    juce::ColourGradient body (Colour (0xff48484c), c.x - rKnob * 0.5f, c.y - rKnob * 0.6f,
                               Colour (0xff050506), c.x + rKnob * 0.7f, c.y + rKnob * 0.8f, true);
    g.setGradientFill (body);
    g.fillEllipse (Rectangle<float> (rKnob * 2.0f, rKnob * 2.0f).withCentre (c));

    // Rillen am Rand
    g.setColour (Colours::white.withAlpha (0.12f));
    for (int i = 0; i < 48; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 48.0f;
        g.drawLine (juce::Line<float> (pt (rKnob * 0.9f, a), pt (rKnob * 0.99f, a)), 1.0f);
    }

    // Kappe
    juce::ColourGradient cap (Colour (0xff35353a), c.x - rKnob * 0.3f, c.y - rKnob * 0.4f,
                              Colour (0xff0b0b0d), c.x + rKnob * 0.4f, c.y + rKnob * 0.5f, true);
    g.setGradientFill (cap);
    g.fillEllipse (Rectangle<float> (rKnob * 1.2f, rKnob * 1.2f).withCentre (c));

    // Zeiger
    juce::Path p;
    p.startNewSubPath (pt (rKnob * 0.12f, angle));
    p.lineTo (pt (rKnob * 0.94f, angle));
    g.setColour (Colour (0xfff4f1e8));
    g.strokePath (p, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void HardwareLook::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    const auto r  = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    const auto kind = b.getProperties()["kind"].toString();

    if (kind == "lamp")
    {
        const float d = juce::jmin (r.getHeight(), 26.0f);
        const auto lamp = Rectangle<float> (d, d).withCentre ({ r.getX() + d * 0.5f + 2.0f, r.getCentreY() });
        const auto lens = lamp.reduced (d * 0.17f);

        if (on)
        {
            juce::ColourGradient glow (Colour (0xffff3b30).withAlpha (0.5f), lamp.getCentreX(), lamp.getCentreY(),
                                       Colour (0xffff3b30).withAlpha (0.0f), lamp.getCentreX() + d * 0.9f, lamp.getCentreY(), true);
            g.setGradientFill (glow);
            g.fillEllipse (lamp.expanded (d * 0.5f));
        }
        g.setColour (Colour (0xffa5a9ad));
        g.fillEllipse (lamp);
        g.setColour (Colour (0xff50555a));
        g.drawEllipse (lamp, 1.2f);
        g.setColour (on ? Colour (0xffff4338) : Colour (0xff4a0f0c));
        g.fillEllipse (lens);
        g.setColour (Colours::white.withAlpha (on ? 0.45f : 0.22f));
        g.fillEllipse (Rectangle<float> (lens.getWidth() * 0.4f, lens.getHeight() * 0.3f)
                           .withCentre ({ lens.getCentreX() - lens.getWidth() * 0.12f,
                                          lens.getCentreY() - lens.getHeight() * 0.2f }));

        g.setColour (textCol());
        g.setFont (mkFont (12.0f, true));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (d + 10.0f), juce::Justification::centredLeft, false);
        return;
    }

    // Kipphebel mit Beschriftung links/rechts
    const auto offTxt = b.getProperties()["textOff"].toString();
    const auto onTxt  = b.getProperties()["textOn"].toString();
    const float cx = r.getCentreX();
    const float cy = r.getCentreY() + 6.0f;

    g.setFont (mkFont (11.0f, true));
    g.setColour (textCol().withAlpha (on ? 0.5f : 1.0f));
    g.drawText (offTxt, r.withWidth (r.getWidth() * 0.5f - 24.0f), juce::Justification::centredRight, false);
    g.setColour (textCol().withAlpha (on ? 1.0f : 0.5f));
    g.drawText (onTxt, r.withTrimmedLeft (r.getWidth() * 0.5f + 24.0f), juce::Justification::centredLeft, false);

    // Sockel (Sechskantmutter)
    juce::ColourGradient nut (Colour (0xfff1f3f4), cx - 9.0f, cy - 9.0f, Colour (0xff6e7277), cx + 9.0f, cy + 9.0f, false);
    g.setGradientFill (nut);
    g.fillEllipse (Rectangle<float> (20.0f, 20.0f).withCentre ({ cx, cy }));
    g.setColour (Colour (0xff3d4045));
    g.drawEllipse (Rectangle<float> (20.0f, 20.0f).withCentre ({ cx, cy }), 1.0f);

    // Hebel
    const Point<float> tip (cx + (on ? 17.0f : -17.0f), cy - 17.0f);
    juce::Path lever;
    lever.startNewSubPath (cx, cy);
    lever.lineTo (tip);
    g.setColour (Colour (0xff2a2c2f));
    g.strokePath (lever, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (Colour (0xffd9dcde));
    g.strokePath (lever, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::ColourGradient ball (Colours::white, tip.x - 2.0f, tip.y - 2.0f, Colour (0xff80848a), tip.x + 4.0f, tip.y + 4.0f, true);
    g.setGradientFill (ball);
    g.fillEllipse (Rectangle<float> (11.0f, 11.0f).withCentre (tip));
}

//==============================================================================
void VuMeter::setValue (float db)
{
    if (std::abs (db - value) > 0.02f)
    {
        value = db;
        repaint();
    }
}

void VuMeter::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0b0c));
    g.fillRoundedRectangle (b, 9.0f);

    auto face = b.reduced (6.0f);
    juce::ColourGradient grad (faceColour.brighter (0.25f), face.getX(), face.getY(),
                               faceColour.darker (0.2f), face.getX(), face.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (face, 4.0f);

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());

    const Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.35f);
    const float L = face.getHeight() * 1.25f;
    const float maxAng = 0.9f;

    auto angFor = [maxAng] (float db) { return juce::jmap (juce::jlimit (0.0f, 20.0f, db), 0.0f, 20.0f, -maxAng, maxAng); };
    auto pt = [&] (float r, float a) { return Point<float> (pivot.x + r * std::sin (a), pivot.y - r * std::cos (a)); };

    g.setFont (mkFont (10.0f, true));
    for (int i = 0; i <= 20; ++i)
    {
        const float a = angFor ((float) i);
        const bool major = (i % 5 == 0);
        const bool red = i >= 15;
        g.setColour (red ? Colour (0xffb3261e) : Colour (0xff222222));
        g.drawLine (juce::Line<float> (pt (L * (major ? 0.84f : 0.88f), a), pt (L * 0.94f, a)), major ? 1.6f : 1.0f);
        if (major)
            g.drawText (juce::String (i), Rectangle<float> (24.0f, 12.0f).withCentre (pt (L * 0.74f, a)),
                        juce::Justification::centred, false);
    }

    g.setColour (Colour (0xff222222));
    g.setFont (mkFont (9.0f, true));
    g.drawText ("GAIN REDUCTION dB", face.withTrimmedTop (face.getHeight() * 0.64f),
                juce::Justification::centredTop, false);

    // Nadel
    const float a = angFor (value);
    g.setColour (Colour (0xff111111));
    g.drawLine (juce::Line<float> (pivot, pt (L * 0.97f, a)), 1.8f);
    g.restoreState();

    // Glasreflexion
    g.setColour (Colours::white.withAlpha (0.08f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.4f), 4.0f);
}

//==============================================================================
ConePressorEditor::ConePressorEditor (ConePressorProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    cat = juce::ImageCache::getFromMemory (BinaryData::cat_png, BinaryData::cat_pngSize);

    const Colour dark (0xff15171a), light (0xfff0f0f0);

    // LA-2A-Panel
    setupKnob (laPeakK, "PEAK REDUCTION", "laPeak", laPeakA, laLook, "0,20,40,60,80,100", dark);
    setupKnob (laGainK, "GAIN",           "laGain", laGainA, laLook, "0,6,12,18,24",      dark);
    setupToggle (laLimitBtn, "laLimit", laLimitA, laLook, "lever", "COMPRESS", "LIMIT");
    setupToggle (laOnBtn,    "laOn",    laOnA,    laLook, "lamp");

    // 670-Panel
    setupKnob (fcInK,     "INPUT",         "fcIn",     fcInA,     fcLook, "-12,0,12,24",     light);
    setupKnob (fcThreshK, "THRESHOLD",     "fcThresh", fcThreshA, fcLook, "-40,-30,-20,-10,0", light);
    setupKnob (fcTimeK,   "TIME CONSTANT", "fcTime",   fcTimeA,   fcLook, "1,2,3,4,5,6",     light);
    setupKnob (fcOutK,    "OUTPUT",        "fcOut",    fcOutA,    fcLook, "-12,0,12,24",     light);
    setupToggle (fcOnBtn, "fcOn", fcOnA, fcLook, "lamp");

    // Mitte (global)
    setupKnob (inK,   "IN",   "in",   inA,   fcLook, "-24,0,24",   light);
    setupKnob (outK,  "OUT",  "out",  outA,  fcLook, "-24,0,24",   light);
    setupKnob (mixK,  "MIX",  "mix",  mixA,  fcLook, "0,50,100",   light);
    setupKnob (tubeK, "TUBE", "tube", tubeA, fcLook, "0,50,100",   light);

    setupToggle (orderBtn, "order", orderA, fcLook, "lever", "LA-2A > 670", "670 > LA-2A");

    addAndMakeVisible (laMeter);
    addAndMakeVisible (fcMeter);

    // Undo/Redo + A/B
    const Colour accent (0xffe8a33d);
    for (auto* b : { &undoBtn, &redoBtn, &abA, &abB, &copyBtn })
    {
        b->setLookAndFeel (&fcLook);
        b->setColour (juce::TextButton::buttonColourId, Colour (0xff2b2f35));
        b->setColour (juce::TextButton::buttonOnColourId, accent);
        b->setColour (juce::TextButton::textColourOffId, Colours::white.withAlpha (0.9f));
        b->setColour (juce::TextButton::textColourOnId, Colour (0xff15171a));
        b->setClickingTogglesState (false);
        addAndMakeVisible (*b);
    }
    undoBtn.onClick = [this] { proc.undoManager.undo(); };
    redoBtn.onClick = [this] { proc.undoManager.redo(); };
    abA.onClick     = [this] { proc.switchToSlot (0); };
    abB.onClick     = [this] { proc.switchToSlot (1); };
    copyBtn.onClick = [this] { proc.copyActiveToOther(); };

    setWantsKeyboardFocus (true);
    setSize (1000, 640);
    startTimerHz (30);
}

ConePressorEditor::~ConePressorEditor()
{
    stopTimer();
}

void ConePressorEditor::setupKnob (Knob& k, const juce::String& text, const juce::String& id,
                                   std::unique_ptr<SA>& att, HardwareLook& look,
                                   const juce::String& scaleLabels, Colour textColour)
{
    k.s.setLookAndFeel (&look);
    k.s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k.s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    k.s.getProperties().set ("labels", scaleLabels);
    k.s.setColour (juce::Slider::textBoxTextColourId, textColour);
    k.s.setColour (juce::Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k.s.setColour (juce::Slider::textBoxBackgroundColourId, Colours::transparentBlack);

    k.l.setText (text, juce::dontSendNotification);
    k.l.setJustificationType (juce::Justification::centred);
    k.l.setFont (mkFont (11.0f, true));
    k.l.setColour (juce::Label::textColourId, textColour);

    addAndMakeVisible (k.s);
    addAndMakeVisible (k.l);
    att = std::make_unique<SA> (proc.apvts, id, k.s);
}

void ConePressorEditor::setupToggle (juce::ToggleButton& b, const juce::String& id, std::unique_ptr<BA>& att,
                                     HardwareLook& look, const juce::String& kind,
                                     const juce::String& textOff, const juce::String& textOn)
{
    b.setLookAndFeel (&look);
    b.getProperties().set ("kind", kind);
    b.getProperties().set ("textOff", textOff);
    b.getProperties().set ("textOn", textOn);
    addAndMakeVisible (b);
    att = std::make_unique<BA> (proc.apvts, id, b);
}

void ConePressorEditor::placeKnob (Knob& k, Rectangle<int> cell)
{
    k.l.setBounds (cell.removeFromTop (18));
    k.s.setBounds (cell);
}

void ConePressorEditor::timerCallback()
{
    laShown = std::max (proc.grLaDb.load(), laShown * 0.88f);
    fcShown = std::max (proc.grFcDb.load(), fcShown * 0.88f);
    laMeter.setValue (laShown);
    fcMeter.setValue (fcShown);

    undoBtn.setEnabled (proc.undoManager.canUndo());
    redoBtn.setEnabled (proc.undoManager.canRedo());
    const int act = proc.getActiveSlot();
    abA.setToggleState (act == 0, juce::dontSendNotification);
    abB.setToggleState (act == 1, juce::dontSendNotification);
    copyBtn.setButtonText (act == 0 ? "A > B" : "B > A");
    repaint (catArea.getUnion (captionArea).expanded (6));
}

void ConePressorEditor::resized()
{
    auto r = getLocalBounds().reduced (14);

    auto top = r.removeFromTop (40);
    orderBtn.setBounds (top.removeFromRight (320).reduced (0, 2));
    top.removeFromLeft (270);   // Platz fuer den Titel
    {
        auto tb = top.reduced (0, 6);
        undoBtn.setBounds (tb.removeFromLeft (62));  tb.removeFromLeft (4);
        redoBtn.setBounds (tb.removeFromLeft (62));  tb.removeFromLeft (24);
        abA.setBounds (tb.removeFromLeft (40));      tb.removeFromLeft (4);
        abB.setBounds (tb.removeFromLeft (40));      tb.removeFromLeft (4);
        copyBtn.setBounds (tb.removeFromLeft (62));
    }
    r.removeFromTop (6);

    laPanel = r.removeFromLeft (330);
    fcPanel = r.removeFromRight (330);
    centerPanel = r.reduced (10, 0);

    // LA-2A
    {
        auto p = laPanel.reduced (14);
        p.removeFromTop (28);
        laMeter.setBounds (p.removeFromTop (110).reduced (30, 0));
        p.removeFromTop (14);
        auto row = p.removeFromTop (170);
        placeKnob (laPeakK, row.removeFromLeft (row.getWidth() / 2));
        placeKnob (laGainK, row);
        p.removeFromTop (6);
        laLimitBtn.setBounds (p.removeFromTop (50).reduced (10, 4));
        p.removeFromTop (10);
        laOnBtn.setBounds (p.removeFromTop (34).withSizeKeepingCentre (150, 30));
    }

    // 670
    {
        auto p = fcPanel.reduced (14);
        p.removeFromTop (28);
        fcMeter.setBounds (p.removeFromTop (110).reduced (30, 0));
        p.removeFromTop (8);
        auto row1 = p.removeFromTop (160);
        placeKnob (fcInK,     row1.removeFromLeft (row1.getWidth() / 2));
        placeKnob (fcThreshK, row1);
        auto row2 = p.removeFromTop (160);
        placeKnob (fcTimeK, row2.removeFromLeft (row2.getWidth() / 2));
        placeKnob (fcOutK,  row2);
        p.removeFromTop (4);
        fcOnBtn.setBounds (p.removeFromTop (34).withSizeKeepingCentre (150, 30));
    }

    // Mitte: Katze + globale Regler
    {
        auto p = centerPanel.reduced (12);
        catArea = p.removeFromTop (230);
        captionArea = p.removeFromTop (28);
        auto row1 = p.removeFromTop (p.getHeight() / 2);
        placeKnob (inK,  row1.removeFromLeft (row1.getWidth() / 2));
        placeKnob (outK, row1);
        placeKnob (mixK,  p.removeFromLeft (p.getWidth() / 2));
        placeKnob (tubeK, p);
    }
}

void ConePressorEditor::drawPanel (juce::Graphics& g, Rectangle<int> area, bool fairchildStyle)
{
    const auto a = area.toFloat();
    juce::ColourGradient grad (fairchildStyle ? Colour (0xff6a6e73) : Colour (0xffd6d9dd), a.getX(), a.getY(),
                               fairchildStyle ? Colour (0xff484c50) : Colour (0xffadb1b7), a.getX(), a.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (a, 10.0f);

    // gebuerstete Oberflaeche
    g.setColour ((fairchildStyle ? Colours::white : Colours::black).withAlpha (0.035f));
    for (float yy = a.getY() + 2.0f; yy < a.getBottom() - 2.0f; yy += 3.0f)
        g.drawHorizontalLine ((int) yy, a.getX() + 4.0f, a.getRight() - 4.0f);

    g.setColour (Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (a, 10.0f, 2.0f);

    // Schrauben
    for (auto pos : { Point<float> (a.getX() + 12.0f, a.getY() + 12.0f), Point<float> (a.getRight() - 12.0f, a.getY() + 12.0f),
                      Point<float> (a.getX() + 12.0f, a.getBottom() - 12.0f), Point<float> (a.getRight() - 12.0f, a.getBottom() - 12.0f) })
    {
        juce::ColourGradient sg (Colour (0xffe6e8ea), pos.x - 4.0f, pos.y - 4.0f, Colour (0xff6c7075), pos.x + 4.0f, pos.y + 4.0f, false);
        g.setGradientFill (sg);
        g.fillEllipse (Rectangle<float> (10.0f, 10.0f).withCentre (pos));
        g.setColour (Colour (0xff2c2f33));
        g.drawLine (pos.x - 3.5f, pos.y + 1.0f, pos.x + 3.5f, pos.y - 1.0f, 1.3f);
    }
}

void ConePressorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (Colour (0xff23262b), 0.0f, 0.0f, Colour (0xff0d0e10), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // Titel
    g.setColour (Colour (0xffe8a33d));
    g.setFont (mkFont (26.0f, true));
    g.drawText ("CONE-PRESSOR", 20, 14, 280, 34, juce::Justification::centredLeft);
    // Panels
    drawPanel (g, laPanel, false);
    drawPanel (g, fcPanel, true);
    drawPanel (g, centerPanel, true);

    g.setColour (Colour (0xff15171a));
    g.setFont (mkFont (15.0f, true));
    g.drawText ("LA-2A  OPTO LEVELING", laPanel.withTrimmedTop (14).withHeight (28), juce::Justification::centred);
    g.setColour (Colours::white.withAlpha (0.92f));
    g.drawText ("670  VARI-MU", fcPanel.withTrimmedTop (14).withHeight (28), juce::Justification::centred);

    // Katze: schrumpft mit der gesamten Gain Reduction
    if (cat.isValid())
    {
        const float scale = 1.0f - juce::jlimit (0.0f, 0.2f, (laShown + fcShown) * 0.008f);
        const auto area = catArea.toFloat();
        const auto drawRect = area.withSizeKeepingCentre (area.getWidth() * scale, area.getHeight() * scale);

        juce::Path clip;
        clip.addRoundedRectangle (drawRect, 16.0f);

        g.saveState();
        g.reduceClipRegion (clip);
        g.drawImage (cat, drawRect, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        g.restoreState();

        g.setColour (Colour (0xffe8a33d).withAlpha (0.85f));
        g.strokePath (clip, juce::PathStrokeType (2.0f));
    }

    g.setColour (Colours::white.withAlpha (0.85f));
    g.setFont (mkFont (12.0f, true));
    g.drawText ("GR  LA-2A -" + juce::String (laShown, 1) + " dB   |   670 -" + juce::String (fcShown, 1) + " dB",
                captionArea, juce::Justification::centred);
}

bool ConePressorEditor::keyPressed (const juce::KeyPress& k)
{
    using KP = juce::KeyPress;
    using MK = juce::ModifierKeys;

    if (k == KP ('z', MK::commandModifier, 0))
    {
        proc.undoManager.undo();
        return true;
    }
    if (k == KP ('z', MK::commandModifier | MK::shiftModifier, 0) || k == KP ('y', MK::commandModifier, 0))
    {
        proc.undoManager.redo();
        return true;
    }
    return false;
}
