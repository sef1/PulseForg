#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/**
 * M1 debug readout: host tempo, transport state, PPQ and the live
 * 16-step position, in the app's hardware-rack visual language.
 */
class PulseForgeEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit PulseForgeEditor (PulseForgeProcessor&);
    ~PulseForgeEditor() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    void timerCallback() override { repaint(); }

    PulseForgeProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeEditor)
};
