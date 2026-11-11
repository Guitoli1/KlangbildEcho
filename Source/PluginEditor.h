#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

class EchoAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit EchoAudioProcessorEditor (EchoAudioProcessor&);
    ~EchoAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Sichtfenster aufs Bandlaufwerk: Band läuft, aktive Köpfe leuchten mit dem Echo
    struct TapeWindow : public juce::Component
    {
        std::array<bool, 3> active { true, false, false };
        std::array<float, 3> level { 0, 0, 0 };
        float offset = 0.0f;
        VintageLookAndFeel* lnf = nullptr;
        void paint (juce::Graphics&) override;
    };

    struct Knob { juce::Slider slider; juce::String name; bool big = false; std::unique_ptr<SliderAttachment> attachment; };
    Knob& addKnob (const juce::String& paramID, const juce::String& name, bool big);
    void setupSwitch (juce::ToggleButton&, const juce::String& paramID, const juce::String& onText,
                      const juce::String& offText, juce::Colour lampColour);
    void timerCallback() override;
    void renderTextures();
    void drawEngraved (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, juce::Justification);

    EchoAudioProcessor& proc;
    VintageLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob* timeKnob = nullptr; Knob* fbKnob = nullptr; Knob* divKnob = nullptr;
    std::array<Knob*, 6> rowKnobs {};
    juce::ToggleButton syncSwitch, stereoSwitch, sendSwitch;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    juce::ComboBox presetBox;
    TapeWindow tapeWin;
    juce::Label timeLabel;

    juce::Rectangle<int> faceplate, presetArea, winBezel, divider, tagArea;
    std::array<juce::Rectangle<int>, 3> switchLabels;
    juce::Image tolexImage, faceplateImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoAudioProcessorEditor)
};
