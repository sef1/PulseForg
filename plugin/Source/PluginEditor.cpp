#include "PluginEditor.h"

namespace
{
const juce::Colour panel      { 0xff1b1b1e };
const juce::Colour panelEdge  { 0xff333338 };
const juce::Colour ledOff     { 0xff3a2023 };
const juce::Colour ledOn      { 0xffff3226 };
const juce::Colour labelDim   { 0xff8f8f99 };
const juce::Colour labelMain  { 0xffe8e8ee };
} // namespace

PulseForgeEditor::PulseForgeEditor (PulseForgeProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p)
{
    setSize (480, 210);
    startTimerHz (30);
}

void PulseForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (panel);

    auto area = getLocalBounds().reduced (16);

    g.setColour (ledOn);
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.drawText ("PULSEFORGE", area.removeFromTop (26), juce::Justification::left);

    g.setColour (labelDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("M2 DSP - demo pattern", area.removeFromTop (16), juce::Justification::left);

    area.removeFromTop (8);

    const bool valid   = processor.hostPositionValid.load();
    const bool playing = processor.hostPlaying.load();
    const auto bpm     = processor.hostBpm.load();
    const auto ppq     = processor.hostPpq.load();
    const int  step    = processor.currentStep.load();

    // 16 step LEDs
    auto ledRow = area.removeFromTop (30);
    const int ledWidth = ledRow.getWidth() / 16;
    for (int i = 0; i < 16; ++i)
    {
        auto box = ledRow.removeFromLeft (ledWidth).reduced (3);
        const bool active = valid && playing && i == step;
        g.setColour (active ? ledOn : ledOff);
        g.fillRect (box);
        g.setColour (panelEdge);
        g.drawRect (box);
    }

    area.removeFromTop (14);

    g.setFont (juce::Font (juce::FontOptions (14.0f)));

    if (! valid)
    {
        g.setColour (labelDim);
        g.drawText ("NO HOST SYNC", area.removeFromTop (20), juce::Justification::left);
    }
    else
    {
        g.setColour (labelMain);
        g.drawText ("BPM " + juce::String (bpm, 1) + "    PPQ " + juce::String (ppq, 2),
                    area.removeFromTop (20), juce::Justification::left);

        g.setColour (playing ? ledOn : labelDim);
        g.drawText (playing ? ("PLAYING   STEP " + juce::String (step + 1) + " / 16") : "STOPPED",
                    area.removeFromTop (20), juce::Justification::left);
    }
}
