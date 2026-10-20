#include "PluginProcessor.h"
#include "PluginEditor.h"

ConePressorProcessor::ConePressorProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ConePressorProcessor::createLayout()
{
    using Attr = juce::AudioParameterFloatAttributes;
    using NR   = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&layout] (const char* id, const char* name, NR range, float def, const char* unit)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def, Attr().withLabel (unit)));
    };
    auto addBool = [&layout] (const char* id, const char* name, bool def)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };

    // Global
    addFloat ("in",   "Input",  NR (-24.0f, 24.0f, 0.1f), 0.0f,   "dB");
    addFloat ("out",  "Output", NR (-24.0f, 24.0f, 0.1f), 0.0f,   "dB");
    addFloat ("mix",  "Mix",    NR (0.0f, 100.0f, 1.0f),  100.0f, "%");
    addFloat ("tube", "Tube",   NR (0.0f, 100.0f, 1.0f),  20.0f,  "%");
    addBool  ("order", "Order 670 first", false);

    // LA-2A-Modul
    addBool  ("laOn",    "LA-2A On",      true);
    addFloat ("laPeak",  "Peak Reduction", NR (0.0f, 100.0f, 0.1f), 30.0f, "");
    addFloat ("laGain",  "LA-2A Gain",     NR (0.0f, 24.0f, 0.1f),  0.0f,  "dB");
    addBool  ("laLimit", "Limit",          false);

    // 670-Modul
    addBool  ("fcOn",     "670 On",         true);
    addFloat ("fcIn",     "670 Input",      NR (-12.0f, 24.0f, 0.1f), 0.0f,   "dB");
    addFloat ("fcThresh", "670 Threshold",  NR (-40.0f, 0.0f, 0.1f),  -16.0f, "dB");
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "fcTime", 1 }, "670 Time Constant",
        juce::StringArray { "1", "2", "3", "4", "5", "6" }, 1));
    addFloat ("fcOut",    "670 Output",     NR (-12.0f, 24.0f, 0.1f), 0.0f,   "dB");

    return layout;
}

void ConePressorProcessor::prepareToPlay (double sampleRate, int)
{
    la.prepare (sampleRate);
    fc.prepare (sampleRate);
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

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float inG  = cp::dbToLin (get ("in"));
    const float outG = cp::dbToLin (get ("out"));
    const float mix  = get ("mix") * 0.01f;
    const float tube = get ("tube") * 0.01f;
    const bool  fcFirst = get ("order") > 0.5f;

    const bool laOn = get ("laOn") > 0.5f;
    const bool fcOn = get ("fcOn") > 0.5f;

    la.setParams (get ("laPeak"), get ("laLimit") > 0.5f, get ("laGain"));
    fc.setParams (get ("fcIn"), get ("fcThresh"), (int) get ("fcTime"), get ("fcOut"));

    if (! laOn) la.reset();
    if (! fcOn) fc.reset();

    float* d0 = buffer.getWritePointer (0);
    float* d1 = nCh > 1 ? buffer.getWritePointer (1) : nullptr;

    float maxLa = 0.0f, maxFc = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float l = d0[i] * inG;
        float r = d1 != nullptr ? d1[i] * inG : l;
        const float dryL = l, dryR = r;

        if (fcFirst)
        {
            if (fcOn) fc.process (l, r);
            if (laOn) la.process (l, r);
        }
        else
        {
            if (laOn) la.process (l, r);
            if (fcOn) fc.process (l, r);
        }

        l = cp::saturate (l, tube);
        r = cp::saturate (r, tube);

        d0[i] = (dryL * (1.0f - mix) + l * mix) * outG;
        if (d1 != nullptr)
            d1[i] = (dryR * (1.0f - mix) + r * mix) * outG;

        maxLa = std::max (maxLa, la.gr);
        maxFc = std::max (maxFc, fc.gr);
    }

    grLaDb.store (maxLa);
    grFcDb.store (maxFc);
}

juce::AudioProcessorEditor* ConePressorProcessor::createEditor()
{
    return new ConePressorEditor (*this);
}

ConePressorProcessor::Snapshot ConePressorProcessor::captureSnapshot() const
{
    Snapshot s;
    for (auto* p : getParameters())
        if (auto* pid = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            s[pid->paramID] = pid->getValue();
    return s;
}

void ConePressorProcessor::applySnapshot (const Snapshot& s)
{
    undoManager.beginNewTransaction ("A/B");

    for (auto* p : getParameters())
    {
        auto* pid = dynamic_cast<juce::AudioProcessorParameterWithID*> (p);
        if (pid == nullptr) continue;

        const auto it = s.find (pid->paramID);
        if (it == s.end()) continue;
        if (std::abs (pid->getValue() - it->second) < 1.0e-6f) continue;

        pid->beginChangeGesture();
        pid->setValueNotifyingHost (it->second);
        pid->endChangeGesture();
    }
}

void ConePressorProcessor::switchToSlot (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    Snapshot target;
    {
        const juce::ScopedLock sl (slotLock);
        const int cur = activeSlot.load();
        if (slot == cur) return;

        slots[(size_t) cur] = captureSnapshot();
        if (slots[(size_t) slot].empty())
            slots[(size_t) slot] = slots[(size_t) cur];   // erster Wechsel: B startet als Kopie von A

        activeSlot.store (slot);
        target = slots[(size_t) slot];
    }
    applySnapshot (target);
}

void ConePressorProcessor::copyActiveToOther()
{
    const juce::ScopedLock sl (slotLock);
    const int cur = activeSlot.load();
    slots[(size_t) (1 - cur)] = captureSnapshot();
}

void ConePressorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("ConePressorSession");
    {
        const juce::ScopedLock sl (slotLock);
        slots[(size_t) activeSlot.load()] = captureSnapshot();
        root.setAttribute ("active", activeSlot.load());

        for (int i = 0; i < 2; ++i)
        {
            auto* e = root.createNewChildElement (i == 0 ? "SlotA" : "SlotB");
            for (const auto& kv : slots[(size_t) i])
                e->setAttribute (kv.first, (double) kv.second);
        }
    }

    if (auto xml = apvts.copyState().createXml())
        root.addChildElement (xml.release());

    copyXmlToBinary (root, destData);
}

void ConePressorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    if (xml->hasTagName ("ConePressorSession"))
    {
        if (auto* st = xml->getChildByName (apvts.state.getType().toString()))
            apvts.replaceState (juce::ValueTree::fromXml (*st));

        const juce::ScopedLock sl (slotLock);
        for (int i = 0; i < 2; ++i)
        {
            slots[(size_t) i].clear();
            if (auto* e = xml->getChildByName (i == 0 ? "SlotA" : "SlotB"))
                for (int a = 0; a < e->getNumAttributes(); ++a)
                {
                    const auto name = e->getAttributeName (a);
                    slots[(size_t) i][name] = (float) e->getDoubleAttribute (name);
                }
        }
        activeSlot.store (juce::jlimit (0, 1, xml->getIntAttribute ("active", 0)));
    }
    else if (xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));   // alter Projektstand ohne A/B
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ConePressorProcessor();
}
