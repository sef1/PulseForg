#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/RackUI.h"
#include "UI/RackPanels.h"

/**
 * M3b editor: the full hardware rack - header (BPM/transport, project
 * export/import), both acid engines, drums, mixer and pattern banks.
 * Sections keep GrooveboxView's internal layout 1:1; the arrangement is
 * two columns because DAW windows are landscape where the app is portrait.
 */
class PulseForgeEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit PulseForgeEditor (PulseForgeProcessor&);
    ~PulseForgeEditor() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Drives one panel-refresh cycle outside the timer (snapshot harness). */
    void refreshRack() { timerCallback(); }

private:
    void timerCallback() override;
    void selectTrack (int t) { selectedTrack = t; }

    PulseForgeProcessor& processor;
    RackLookAndFeel lnf; // before the panels: installs itself as default
    int selectedTrack = 0; // 0/1 acid A/B, 2/3 drum 8/9 (GrooveboxView semantics)

    HeaderPanel header;
    SynthPanel synthA, synthB;
    DrumPanel drums;
    MixerPanel mixer;
    FooterPanel footer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeEditor)
};
