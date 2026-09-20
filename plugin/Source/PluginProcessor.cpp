#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TransportSync.h"
#include "BlockSequencer.h"

#include <cmath>

PulseForgeProcessor::PulseForgeProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    pattern = pulseforge::Pattern::demo();
    engine.setPattern (pattern);
}

void PulseForgeProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate);
    sequencer.wasPlaying = false;
}

bool PulseForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::disabled() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled();
}

void PulseForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();

    bool valid = false, playing = false;
    double bpm = 0.0, ppq = 0.0;

    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            valid = true;
            playing = position->getIsPlaying();
            if (const auto b = position->getBpm()) bpm = *b;
            if (const auto p = position->getPpqPosition()) ppq = *p;
        }
    }

    hostPositionValid = valid;
    hostPlaying = playing;
    if (valid)
    {
        hostBpm = bpm;
        hostPpq = ppq;
        currentStep = pulseforge::TransportStep::stepIndexForPpq (ppq);
    }

    const int numSamples = buffer.getNumSamples();
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);

    if (! sequencer.renderBlock (engine, currentSampleRate, valid, playing,
                                 bpm, ppq, outL, outR, numSamples))
        buffer.clear();
}

juce::AudioProcessorEditor* PulseForgeProcessor::createEditor()
{
    return new PulseForgeEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PulseForgeProcessor();
}
