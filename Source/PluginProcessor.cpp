#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;
juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
Attr withText (std::function<juce::String (float)> f)
{
    return Attr().withStringFromValueFunction ([f] (float v, int) { return f (v); })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
}
Attr s10() { return withText ([] (float v) { return juce::String (v, 1); }); }
Attr pct() { return withText ([] (float v) { return juce::String (juce::roundToInt (v)) + " %"; }); }
}

juce::AudioProcessorValueTreeState::ParameterLayout EchoAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<APF> (juce::ParameterID { "time", 1 }, "Zeit", juce::NormalisableRange<float> (40.0f, 700.0f, 1.0f, 0.6f), 300.0f,
        withText ([] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; })));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "sync", 1 }, "Im Takt", false));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "division", 1 }, "Notenwert",
        juce::StringArray { "1/4", "1/8 punktiert", "1/8", "1/8 Triole", "1/16" }, 2));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "heads", 1 }, u8 ("K\xc3\xb6pfe"),
        juce::StringArray { "1", "2", "3", "1+2", "2+3", "1+3", "1+2+3" }, 0));
    l.add (std::make_unique<APF> (juce::ParameterID { "feedback", 1 }, u8 ("R\xc3\xbc" "ckkopplung"), juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.5f, s10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "age", 1 }, "Bandalter", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f, s10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "drive", 1 }, u8 ("S\xc3\xa4ttigung"), juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f, s10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "tone", 1 }, "Ton", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f, s10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "mix", 1 }, "Mischung", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 30.0f, pct()));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "stereo", 1 }, "Stereo", false));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "send", 1 }, "Nur Echo (Send)", false));
    return l;
}

const std::vector<EchoAudioProcessor::Preset>& EchoAudioProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Standard",                           { { "time", 300 }, { "sync", 0 }, { "division", 2 }, { "heads", 0 }, { "feedback", 3.5f }, { "age", 3 },   { "drive", 3 },   { "tone", 5 },   { "mix", 30 }, { "stereo", 0 }, { "send", 0 } } },
        { "Slapback",                           { { "time", 110 }, { "sync", 0 }, { "division", 2 }, { "heads", 0 }, { "feedback", 0.5f }, { "age", 2 },   { "drive", 4 },   { "tone", 6 },   { "mix", 35 }, { "stereo", 0 }, { "send", 0 } } },
        { "Soul \xe2\x80\x93 Achtel im Takt",    { { "time", 300 }, { "sync", 1 }, { "division", 2 }, { "heads", 0 }, { "feedback", 3.0f }, { "age", 2.5f },{ "drive", 3 },   { "tone", 4.5f },{ "mix", 22 }, { "stereo", 0 }, { "send", 0 } } },
        { "Blues \xe2\x80\x93 warmes Echo",      { { "time", 380 }, { "sync", 0 }, { "division", 2 }, { "heads", 0 }, { "feedback", 4.0f }, { "age", 4 },   { "drive", 5 },   { "tone", 3 },   { "mix", 25 }, { "stereo", 0 }, { "send", 0 } } },
        { "Dreikopf-Rhythmus",                  { { "time", 180 }, { "sync", 0 }, { "division", 4 }, { "heads", 6 }, { "feedback", 3.0f }, { "age", 3 },   { "drive", 3 },   { "tone", 5.5f },{ "mix", 30 }, { "stereo", 1 }, { "send", 0 } } },
        { "Punktierte Achtel",                  { { "time", 300 }, { "sync", 1 }, { "division", 1 }, { "heads", 0 }, { "feedback", 4.0f }, { "age", 2 },   { "drive", 2 },   { "tone", 6 },   { "mix", 30 }, { "stereo", 1 }, { "send", 0 } } },
        { "Altes Band \xe2\x80\x93 leiernd",     { { "time", 340 }, { "sync", 0 }, { "division", 2 }, { "heads", 3 }, { "feedback", 4.5f }, { "age", 9 },   { "drive", 6 },   { "tone", 2.5f },{ "mix", 30 }, { "stereo", 0 }, { "send", 0 } } },
        { "Dub \xe2\x80\x93 kurz vorm Kippen",   { { "time", 420 }, { "sync", 0 }, { "division", 1 }, { "heads", 0 }, { "feedback", 8.3f }, { "age", 5 },   { "drive", 6 },   { "tone", 3.5f },{ "mix", 35 }, { "stereo", 0 }, { "send", 0 } } },
        { "Send \xe2\x80\x93 100 % Echo",        { { "time", 320 }, { "sync", 0 }, { "division", 2 }, { "heads", 0 }, { "feedback", 3.5f }, { "age", 3 },   { "drive", 3 },   { "tone", 5 },   { "mix", 100 },{ "stereo", 1 }, { "send", 1 } } },
    };
    return p;
}

void EchoAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

const juce::String EchoAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

EchoAudioProcessor::EchoAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildEcho", createLayout())
{
}

bool EchoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void EchoAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    tape.prepare ((int) (3.0 * maxBaseSeconds * 1.05 * sr) + 64);
    loopHp.setCutoff (110.0, sr);
    for (auto* f : { &loopHp, &loopLp, &loopLp2 }) f->reset();
    lastTone = lastAge = -1.0f;
    delayCur = apvts.getRawParameterValue ("time")->load() * 0.001 * sr;
    glide = 1.0f - (float) std::exp (-1.0 / (0.12 * sr));           // Motor-Trägheit ~120 ms
    wowPhase = flutterPhase = driftPhase = 0.0;
    env = { 0, 0, 0 };
    for (auto* s : { &smFb, &smDrive, &smMix, &smDry, &smStereo }) s->reset (sr, 0.04);
    smFb.setCurrentAndTargetValue (apvts.getRawParameterValue ("feedback")->load() * 0.115f);
    smDrive.setCurrentAndTargetValue (1.0f + 0.3f * apvts.getRawParameterValue ("drive")->load());
    smMix.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load() * 0.01f);
    smDry.setCurrentAndTargetValue (apvts.getRawParameterValue ("send")->load() > 0.5f ? 0.0f : 1.0f);
    smStereo.setCurrentAndTargetValue (apvts.getRawParameterValue ("stereo")->load());
}

void EchoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = juce::jmin (2, getTotalNumOutputChannels());
    if (n == 0 || outCh == 0)
        return;
    if (inCh == 1 && outCh == 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);

    // Zeit: frei oder im Songtempo
    double baseSec = apvts.getRawParameterValue ("time")->load() * 0.001;
    if (apvts.getRawParameterValue ("sync")->load() > 0.5f)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm()) bpm = *b;
        baseSec = beatsPerRepeat[(size_t) juce::jlimit (0, 4, (int) apvts.getRawParameterValue ("division")->load())] * 60.0 / juce::jmax (20.0, bpm);
        while (baseSec > maxBaseSeconds) baseSec *= 0.5;            // sehr langsame Tempi: halbieren
    }
    baseSec = juce::jlimit (0.03, maxBaseSeconds, baseSec);
    currentMs.store ((float) (baseSec * 1000.0));
    const double target = baseSec * sr;

    bool active[3];
    echo::headMask ((int) apvts.getRawParameterValue ("heads")->load(), active);
    const int numActive = (int) active[0] + (int) active[1] + (int) active[2];
    const float norm = 1.0f / std::sqrt ((float) numActive);

    const float age = apvts.getRawParameterValue ("age")->load() * 0.1f, tone = apvts.getRawParameterValue ("tone")->load() * 0.1f;
    if (! juce::exactlyEqual (age, lastAge) || ! juce::exactlyEqual (tone, lastTone))
    {
        // älteres Band verliert Höhen
        const double fc = 1400.0 * std::pow (6.0, (double) tone) * (1.0 - 0.45 * age);
        loopLp.setCutoff (fc, sr);
        loopLp2.setCutoff (fc * 1.6, sr);
        lastAge = age; lastTone = tone;
    }
    const float wowDepth = 0.0004f + 0.0045f * age * age, flutterDepth = 0.00008f + 0.0009f * age * age;

    smFb.setTargetValue (apvts.getRawParameterValue ("feedback")->load() * 0.115f);        // bis 1,15: kippt in Selbstoszillation
    smDrive.setTargetValue (1.0f + 0.3f * apvts.getRawParameterValue ("drive")->load());
    smMix.setTargetValue (apvts.getRawParameterValue ("mix")->load() * 0.01f);
    smDry.setTargetValue (apvts.getRawParameterValue ("send")->load() > 0.5f ? 0.0f : 1.0f);
    smStereo.setTargetValue (apvts.getRawParameterValue ("stereo")->load() > 0.5f ? 1.0f : 0.0f);

    float* L = buffer.getWritePointer (0);
    float* R = outCh > 1 ? buffer.getWritePointer (1) : nullptr;
    constexpr double twoPi = 6.283185307179586;
    static constexpr float pan[3] = { -0.6f, 0.6f, 0.0f };
    const float envCoef = 1.0f - (float) std::exp (-1.0 / (0.05 * sr));

    for (int i = 0; i < n; ++i)
    {
        const float dl = L[i], dr = R ? R[i] : dl;
        delayCur += (target - delayCur) * glide;
        wowPhase += 0.55 / sr;      if (wowPhase >= 1.0) wowPhase -= 1.0;
        flutterPhase += 7.3 / sr;   if (flutterPhase >= 1.0) flutterPhase -= 1.0;
        driftPhase += 0.13 / sr;    if (driftPhase >= 1.0) driftPhase -= 1.0;
        const float wow = wowDepth * (float) (std::sin (twoPi * wowPhase) + 0.5 * std::sin (twoPi * driftPhase));
        const float modL = 1.0f + wow + flutterDepth * (float) std::sin (twoPi * flutterPhase);
        const float st = smStereo.getNextValue();
        // rechter Kanal liest mit versetzter Gleichlaufschwankung -> Breite
        const float modR = 1.0f + wow * (1.0f - 0.6f * st) + flutterDepth * (float) std::sin (twoPi * flutterPhase + 1.9 * st)
                                + st * 0.0006f * (float) std::sin (twoPi * driftPhase * 3.0 + 1.0);

        float sum = 0.0f, wl = 0.0f, wr = 0.0f;
        for (int k = 0; k < 3; ++k)
        {
            headGain[(size_t) k] += ((active[k] ? 1.0f : 0.0f) - headGain[(size_t) k]) * 0.002f;
            const float g = headGain[(size_t) k];
            if (g < 1.0e-4f) { env[(size_t) k] *= 0.999f; continue; }
            const float d = juce::jmax (4.0f, (float) delayCur * (float) (k + 1));
            const float a = tape.read (d * modL);
            const float b = st > 0.001f ? tape.read (d * modR) : a;
            sum += g * a;
            const float p = pan[k] * st;
            wl += g * a * (1.0f - juce::jmax (0.0f, p));
            wr += g * b * (1.0f + juce::jmin (0.0f, p));
            env[(size_t) k] += (std::abs (a) * g - env[(size_t) k]) * envCoef;
        }

        // Aufnahme: Eingang + Rückkopplung, Bandsättigung, Bandfilter
        const float g = smDrive.getNextValue();
        float rec = 0.5f * (dl + dr) + smFb.getNextValue() * norm * sum;
        rec = std::tanh (rec * g) / g;
        rec = loopLp2.lp (loopLp.lp (loopHp.hp (rec)));
        tape.push (rec);

        const float wet = smMix.getNextValue() * norm * std::pow (g, 0.35f) * 1.25f, dry = smDry.getNextValue();
        if (R) { L[i] = dry * dl + wet * wl; R[i] = dry * dr + wet * wr; }
        else     L[i] = dry * dl + wet * 0.5f * (wl + wr);
    }
    for (size_t k = 0; k < 3; ++k) headLevel[k].store (env[k]);
}

void EchoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void EchoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* EchoAudioProcessor::createEditor() { return new kb::ScaledEditor (*this, new EchoAudioProcessorEditor (*this)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EchoAudioProcessor(); }
