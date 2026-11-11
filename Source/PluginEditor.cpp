#include "PluginEditor.h"

using namespace vintage;

namespace { juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); } }

//==============================================================================
EchoAudioProcessorEditor::EchoAudioProcessorEditor (EchoAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    timeKnob = &addKnob ("time", "ZEIT", true);
    fbKnob   = &addKnob ("feedback", u8 ("R\xc3\x9c" "CKKOPPLUNG"), true);
    timeKnob->slider.getProperties().set ("stepLabels", "40|110|240|440|700 MS");
    rowKnobs = { &addKnob ("heads", u8 ("K\xc3\x96PFE"), false), &addKnob ("division", "NOTENWERT", false), &addKnob ("age", "BANDALTER", false),
                 &addKnob ("drive", u8 ("S\xc3\x84TTIGUNG"), false), &addKnob ("tone", "TON", false), &addKnob ("mix", "MISCHUNG", false) };
    divKnob = rowKnobs[1];
    rowKnobs[0]->slider.getProperties().set ("stepLabels", "1|2|3|1+2|2+3|1+3|ALLE");
    rowKnobs[0]->slider.setMouseDragSensitivity (150);
    rowKnobs[1]->slider.getProperties().set ("stepLabels", "1/4|1/8.|1/8|1/8T|1/16");
    rowKnobs[1]->slider.setMouseDragSensitivity (130);

    setupSwitch (syncSwitch, "sync", "IM TAKT", "FREI", lampAmber);
    setupSwitch (stereoSwitch, "stereo", "STEREO", "MONO", lampRed);
    setupSwitch (sendSwitch, "send", "NUR ECHO", "MIT DIREKT", lampAmber);

    presetBox.getProperties().set ("plate", true);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    tapeWin.lnf = &lnf;
    tapeWin.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (tapeWin);
    timeLabel.setJustificationType (juce::Justification::centred);
    timeLabel.setFont (lnf.labelFont (15.0f, 0.15f));
    timeLabel.setColour (juce::Label::textColourId, ink);
    timeLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (timeLabel);

    setSize (1180, 650);
    startTimerHz (40);
}

EchoAudioProcessorEditor::~EchoAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

EchoAudioProcessorEditor::Knob& EchoAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& name, bool big)
{
    auto k = std::make_unique<Knob>();
    k->name = name;
    k->big = big;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setMouseDragSensitivity (big ? 320 : 220);
    addAndMakeVisible (k->slider);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    if (auto* rp = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void EchoAudioProcessorEditor::setupSwitch (juce::ToggleButton& b, const juce::String& paramID,
                                            const juce::String& onText, const juce::String& offText, juce::Colour lampColour)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lampColour.getARGB());
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, b));
}

//==============================================================================
void EchoAudioProcessorEditor::renderTextures()
{
    tolexImage = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), false);
    {
        juce::Image::BitmapData data (tolexImage, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (11);
        for (int y = 0; y < tolexImage.getHeight(); ++y)
            for (int x = 0; x < tolexImage.getWidth(); ++x)
            {
                const int weave = ((x + y) % 4 == 0 || (x - y + 4096) % 4 == 0) ? -5 : 0;
                const int n = rnd.nextInt (13) - 6 + weave;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, 34 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 28 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 24 + n)));
            }
    }
    if (faceplate.isEmpty())
        return;
    faceplateImage = juce::Image (juce::Image::ARGB, faceplate.getWidth(), faceplate.getHeight(), true);
    juce::Graphics g (faceplateImage);
    const auto r = faceplateImage.getBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (cream.brighter (0.06f), 0, 0, creamDark, 0, r.getHeight(), false));
    g.fillRoundedRectangle (r, 6.0f);
    juce::Random rnd (5);
    for (int y = 0; y < faceplate.getHeight(); ++y)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xff6b5534)).withAlpha (0.015f + rnd.nextFloat() * 0.035f));
        g.drawHorizontalLine (y, 0.0f, r.getWidth());
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                             juce::Colour (0x30694a1c), 0, 0, true));
    g.fillRoundedRectangle (r, 6.0f);
}

void EchoAudioProcessorEditor::drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                                             juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawFittedText (text, r.translated (0.0f, 1.0f).toNearestInt(), j, 2, 1.0f);
    g.setColour (ink);
    g.drawFittedText (text, r.toNearestInt(), j, 2, 1.0f);
}

void EchoAudioProcessorEditor::timerCallback()
{
    const float ms = proc.currentMs.load();
    // Band läuft schneller, wenn die Zeit kürzer ist
    tapeWin.offset = std::fmod (tapeWin.offset + juce::jlimit (1.0f, 12.0f, 900.0f / ms), 24.0f);
    echo::headMask ((int) proc.apvts.getRawParameterValue ("heads")->load(), tapeWin.active.data());
    for (size_t k = 0; k < 3; ++k)
        tapeWin.level[k] = juce::jlimit (0.0f, 1.0f, proc.headLevel[k].load() * 8.0f);
    tapeWin.repaint();

    const bool sync = proc.apvts.getRawParameterValue ("sync")->load() > 0.5f;
    timeKnob->slider.setAlpha (sync ? 0.45f : 1.0f);
    divKnob->slider.setAlpha (sync ? 1.0f : 0.45f);
    const auto txt = juce::String (juce::roundToInt (ms)) + " ms";
    if (timeLabel.getText() != txt)
        timeLabel.setText (txt, juce::dontSendNotification);

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void EchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (tolexImage, 0, 0);

    const auto scriptF = lnf.scriptFont (64.0f);
    const auto logoArea = juce::Rectangle<float> (40.0f, 8.0f, 400.0f, 84.0f);
    g.setFont (scriptF);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("Klangbild", logoArea.translated (2.0f, 3.0f), juce::Justification::centredLeft);
    g.setColour (logo);
    g.drawText ("Klangbild", logoArea, juce::Justification::centredLeft);
    const float sw = (float) juce::GlyphArrangement::getStringWidthInt (scriptF, "Klangbild");
    g.setColour (gold);
    g.setFont (lnf.labelFont (26.0f, 0.35f));
    g.drawText ("ECHO", juce::Rectangle<float> (logoArea.getX() + sw + 14.0f, logoArea.getY() + 30.0f, 160.0f, 40.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.55f));
    g.setFont (lnf.labelFont (12.0f, 0.3f));
    g.drawText (u8 ("BANDECHO \xc2\xb7 DREI K\xc3\x96PFE \xc2\xb7 WOW & FLUTTER \xc2\xb7 BANDS\xc3\x84TTIGUNG"),
                juce::Rectangle<float> (44.0f, 90.0f, 600.0f, 16.0f), juce::Justification::centredLeft);
    const auto pa = presetArea.toFloat();
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText ("PROGRAMM", pa.withY (pa.getY() - 15.0f).withHeight (13.0f), juce::Justification::centredLeft);

    const auto fp = faceplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (fp.expanded (4.0f).translated (0, 2.0f), 9.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.3f), 0, fp.getY(), gold.darker (0.6f), 0, fp.getBottom(), false));
    g.fillRoundedRectangle (fp.expanded (3.0f), 8.0f);
    g.drawImageAt (faceplateImage, faceplate.getX(), faceplate.getY());
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (fp, 6.0f, 1.0f);
    for (auto [x, y, a] : { std::tuple { fp.getX() + 12.0f, fp.getY() + 12.0f, 0.4f }, std::tuple { fp.getRight() - 12.0f, fp.getY() + 12.0f, 1.3f },
                            std::tuple { fp.getX() + 12.0f, fp.getBottom() - 12.0f, 2.2f }, std::tuple { fp.getRight() - 12.0f, fp.getBottom() - 12.0f, 0.9f } })
        VintageLookAndFeel::drawScrew (g, { x, y }, 5.0f, a);

    const auto wb = winBezel.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (wb.translated (2.0f, 3.0f), 12.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.25f), 0, wb.getY(), gold.darker (0.55f), 0, wb.getBottom(), false));
    g.fillRoundedRectangle (wb, 12.0f);
    for (auto pt : { wb.getTopLeft().translated (11, 11), wb.getTopRight().translated (-11, 11),
                     wb.getBottomLeft().translated (11, -11), wb.getBottomRight().translated (-11, -11) })
        VintageLookAndFeel::drawScrew (g, pt, 4.0f, pt.x * 0.013f);

    const auto dv = divider.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawHorizontalLine ((int) dv.getY() + 1, dv.getX(), dv.getRight());
    g.setColour (ink.withAlpha (0.45f));
    g.drawHorizontalLine ((int) dv.getY(), dv.getX(), dv.getRight());

    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        drawEngraved (g, k->name, { sb.getX() - 20.0f, sb.getBottom() - 4.0f, sb.getWidth() + 40.0f, k->big ? 24.0f : 18.0f },
                      lnf.labelFont (k->big ? 18.0f : 12.0f, k->big ? 0.2f : 0.12f), juce::Justification::centred);
    }
    const char* names[] = { "TEMPO", "STEREO-BILD", "AUSGANG" };
    for (size_t i = 0; i < 3; ++i)
        drawEngraved (g, names[i], switchLabels[i].toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);

    const auto tag = tagArea.toFloat();
    g.setColour (ink.withAlpha (0.85f));
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (gold);
    g.setFont (lnf.labelFont (11.0f, 0.2f));
    g.drawText ("MODELL KB-201", tag, juce::Justification::centred);
}

void EchoAudioProcessorEditor::resized()
{
    presetArea = { 44, 128, 300, 32 };
    presetBox.setBounds (presetArea);
    faceplate = { 24, 178, getWidth() - 48, getHeight() - 202 };
    const auto in = faceplate.reduced (34, 22);

    timeKnob->slider.setBounds (in.getX() + 20, in.getY() + 6, 220, 200);
    fbKnob->slider.setBounds (in.getRight() - 240, in.getY() + 6, 220, 200);
    winBezel = juce::Rectangle<int> (460, 200).withCentre ({ in.getCentreX(), in.getY() + 104 });
    tapeWin.setBounds (winBezel.reduced (16).withTrimmedBottom (24));
    timeLabel.setBounds (winBezel.getX() + 16, winBezel.getBottom() - 36, winBezel.getWidth() - 32, 22);

    divider = { in.getX() + 10, in.getY() + 234, in.getWidth() - 20, 2 };

    const int ry = in.getY() + 256;
    constexpr int kw = 108, kh = 108, rowW = 700;
    for (int i = 0; i < 6; ++i)
        rowKnobs[(size_t) i]->slider.setBounds (in.getX() + i * (rowW / 6) + (rowW / 6 - kw) / 2, ry, kw, kh);

    const int sx = in.getX() + rowW + 24;
    switchLabels[0] = { sx, ry, 170, 16 };
    syncSwitch.setBounds (sx, ry + 18, 165, 40);
    switchLabels[1] = { sx + 178, ry, 170, 16 };
    stereoSwitch.setBounds (sx + 178, ry + 18, 160, 40);
    switchLabels[2] = { sx, ry + 70, 170, 16 };
    sendSwitch.setBounds (sx, ry + 88, 190, 40);
    tagArea = juce::Rectangle<int> (130, 24).withPosition (in.getRight() - 130, ry + 96);
    renderTextures();
}

//==============================================================================
void EchoAudioProcessorEditor::TapeWindow::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff24211d), 0, b.getY(), juce::Colour (0xff0d0c0a), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 4.0f);

    // Kopfträger-Platte
    const auto deck = b.reduced (10.0f, 0.0f).withTop (b.getCentreY() - 6.0f).withBottom (b.getBottom() - 8.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a5e63), 0, deck.getY(), juce::Colour (0xff2c2e31), 0, deck.getBottom(), false));
    g.fillRoundedRectangle (deck, 4.0f);

    // Band: läuft von links nach rechts über die Köpfe
    const float ty = b.getY() + b.getHeight() * 0.34f, th = 15.0f;
    const auto tapeR = juce::Rectangle<float> (b.getX(), ty, b.getWidth(), th);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b4a2c), 0, ty, juce::Colour (0xff3d2815), 0, ty + th, false));
    g.fillRect (tapeR);
    g.setColour (juce::Colour (0xffb98a55).withAlpha (0.35f));
    for (float x = b.getX() - 24.0f + offset; x < b.getRight(); x += 24.0f)
        g.fillRect (x, ty + 3.0f, 9.0f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillRect (tapeR.withHeight (1.5f));

    // Führungsrollen
    for (float x : { b.getX() + 22.0f, b.getRight() - 22.0f })
    {
        const auto r = juce::Rectangle<float> (26.0f, 26.0f).withCentre ({ x, ty + th + 12.0f });
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd4d7db), r.getX(), r.getY(), juce::Colour (0xff5a5d61), r.getRight(), r.getBottom(), false));
        g.fillEllipse (r);
        g.setColour (juce::Colour (0xff1c1c1c));
        g.fillEllipse (r.reduced (9.0f));
        const float a = offset / 24.0f * juce::MathConstants<float>::twoPi;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawLine (x, ty + th + 12.0f, x + 10.0f * std::cos (a), ty + th + 12.0f + 10.0f * std::sin (a), 1.5f);
    }

    // Köpfe: Aufnahme + drei Wiedergabeköpfe
    const float x0 = b.getX() + 80.0f, dx = (b.getWidth() - 160.0f) / 3.0f;
    const char* labels[] = { "AUFN.", "1", "2", "3" };
    for (int i = 0; i < 4; ++i)
    {
        const float cx = x0 + dx * (float) i;
        const bool rec = i == 0, on = rec || active[(size_t) (i - 1)];
        const float lv = rec ? 0.0f : level[(size_t) (i - 1)];
        const auto head = juce::Rectangle<float> (34.0f, 36.0f).withCentre ({ cx, ty + th + 16.0f });
        juce::Path hp;
        hp.addRoundedRectangle (head.getX(), head.getY(), head.getWidth(), head.getHeight(), 9.0f, 9.0f, true, true, false, false);
        g.setGradientFill (juce::ColourGradient (juce::Colour (on ? 0xffe2e5e9 : 0xff8a8d91), head.getX(), 0, juce::Colour (on ? 0xff7d8186 : 0xff4a4c4f), head.getRight(), 0, false));
        g.fillPath (hp);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillRect (cx - 0.75f, head.getY(), 1.5f, 9.0f);       // Kopfspalt

        // Lampe unter dem Kopf
        const auto lamp = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ cx, head.getBottom() + 14.0f });
        const juce::Colour col = rec ? juce::Colour (0xffff4a32) : juce::Colour (0xffffb23c);
        const float glow = rec ? 0.9f : (on ? 0.35f + 0.65f * lv : 0.0f);
        if (glow > 0.0f)
        {
            g.setColour (col.withAlpha (0.3f * glow));
            g.fillEllipse (lamp.expanded (6.0f + 6.0f * lv));
        }
        g.setColour (col.withAlpha (0.15f + 0.85f * glow));
        g.fillEllipse (lamp);
        g.setColour (juce::Colour (0xffd9c48f).withAlpha (on ? 0.9f : 0.4f));
        g.setFont (lnf != nullptr ? lnf->labelFont (10.0f, 0.15f) : juce::Font (juce::FontOptions (10.0f)));
        g.drawText (labels[i], juce::Rectangle<float> (60.0f, 12.0f).withCentre ({ cx, lamp.getBottom() + 11.0f }), juce::Justification::centred);
    }

    juce::Path refl;
    refl.addTriangle (b.getX(), b.getY(), b.getX() + b.getWidth() * 0.4f, b.getY(), b.getX(), b.getY() + b.getHeight() * 0.7f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillPath (refl);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 2.0f);
}
