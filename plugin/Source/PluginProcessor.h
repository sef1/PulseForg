#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/PulseForgeDSP.h"
#include "BlockSequencer.h"
#include <atomic>

/**
 * PulseForge VST3 - M3a milestone.
 *
 * M2 DSP plus:
 *  - 47 host-automatable parameters: both acid engines (13 each), the four
 *    mixer channels (5 each) and the master drive. They map 1:1 onto the
 *    engine's SynthParams/MixerChannel fields and are applied every block.
 *  - DAW project state: getStateInformation/setStateInformation serialize
 *    the Android ProjectStore JSON document (pattern banks, bank index,
 *    synth and mixer settings), so sessions round-trip in Cubase and
 *    projects stay interchangeable with the Android app.
 */
class PulseForgeProcessor : public juce::AudioProcessor
{
public:
    PulseForgeProcessor();
    ~PulseForgeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Transport readout state: written on the audio thread, read on the UI thread.
    std::atomic<bool>   hostPositionValid { false };
    std::atomic<bool>   hostPlaying { false };
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> hostPpq { 0.0 };
    std::atomic<int>    currentStep { 0 }; // 0-15

    juce::AudioProcessorValueTreeState apvts;

    // Pattern banks (message/audio shared; bank editing UI lands in M3b).
    pulseforge::Pattern banks[8];
    int currentBank = 0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float paramValue (const char* id) const;
    void updateEngineFromParameters();
    void setParamFromState (const char* id, double value01);

    pulseforge::PulseForgeEngine engine;
    pulseforge::BlockSequencer   sequencer;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeProcessor)
};
