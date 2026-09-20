#include "PluginEditor.h"

PulseForgeEditor::PulseForgeEditor (PulseForgeProcessor& p)
    : juce::AudioProcessorEditor (p),
      processor (p),
      header (p),
      synthA (p, 0, [this] (int t) { selectTrack (t); }),
      synthB (p, 1, [this] (int t) { selectTrack (t); }),
      drums (p, [this] (int t) { selectTrack (t); }),
      mixer (p, [this] (int t) { selectTrack (t); }),
      footer (p)
{
    addAndMakeVisible (header);
    addAndMakeVisible (synthA);
    addAndMakeVisible (synthB);
    addAndMakeVisible (drums);
    addAndMakeVisible (mixer);
    addAndMakeVisible (footer);
    setSize (1020, 640);
    startTimerHz (30);
}

void PulseForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (rack::darkBg);
}

void PulseForgeEditor::resized()
{
    auto area = getLocalBounds().reduced (14);

    header.setBounds (area.removeFromTop (64));
    area.removeFromTop (10);

    auto cols = area.removeFromTop (400);
    auto left = cols.removeFromLeft ((cols.getWidth() - 12) / 2);
    cols.removeFromLeft (12);

    synthA.setBounds (left.removeFromTop (195));
    left.removeFromTop (10);
    synthB.setBounds (left);

    drums.setBounds (cols.removeFromTop (195));
    cols.removeFromTop (10);
    mixer.setBounds (cols);

    area.removeFromTop (10);
    footer.setBounds (area);
}

void PulseForgeEditor::timerCallback()
{
    header.update();
    synthA.update (selectedTrack);
    synthB.update (selectedTrack);
    drums.update (selectedTrack);
    mixer.update (selectedTrack);
    footer.update();
}
