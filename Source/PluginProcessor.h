#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "EchoDsp.h"

class EchoAudioProcessor : public juce::AudioProcessor
{
public:
    EchoAudioProcessor();
    ~EchoAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();
    static constexpr std::array<double, 5> beatsPerRepeat { 1.0, 0.75, 0.5, 1.0 / 3.0, 0.25 };
    static constexpr double maxBaseSeconds = 0.7;
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>, 3> headLevel { 0.0f, 0.0f, 0.0f };   // für die Lampen im Fenster
    std::atomic<float> currentMs { 300.0f };                           // tatsächliche Zeit von Kopf 1

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    echo::TapeLine tape;
    echo::OnePole loopHp, loopLp, loopLp2;
    double delayCur = 0.0, wowPhase = 0.0, flutterPhase = 0.0, driftPhase = 0.0;
    float glide = 0.001f;
    std::array<float, 3> headGain { 1, 0, 0 }, env { 0, 0, 0 };
    float lastTone = -1.0f, lastAge = -1.0f;
    juce::SmoothedValue<float> smFb, smDrive, smMix, smDry, smStereo;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoAudioProcessor)
};
