#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/PulseForgeDSP.h"
#include "BlockSequencer.h"
#include <atomic>

/**
 * PulseForge VST3 - M2 milestone.
 *
 * Full DSP port running on the host transport: acid engines A/B, drum 8/9,
 * mixer, send delay and drive, sequenced sample-accurately from the host
 * PPQ position. M2 plays the built-in demo pattern; parameters, banks and
 * project state land in M3.
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

    // M2: no meaningful state yet. Project JSON lands in M3 via these hooks.
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // Transport readout state: written on the audio thread, read on the UI thread.
    std::atomic<bool>   hostPositionValid { false };
    std::atomic<bool>   hostPlaying { false };
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> hostPpq { 0.0 };
    std::atomic<int>    currentStep { 0 }; // 0-15

private:
    pulseforge::Pattern          pattern;
    pulseforge::PulseForgeEngine engine;
    pulseforge::BlockSequencer   sequencer;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeProcessor)
};
