#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TransportSync.h"

PulseForgeProcessor::PulseForgeProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void PulseForgeProcessor::prepareToPlay (double, int) {}

bool PulseForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();

    // Synth: no input bus, stereo output (a disabled output bus is allowed
    // so hosts and validators can query layouts during setup).
    if (mainOut != juce::AudioChannelSet::disabled() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled();
}

void PulseForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // M1: silent output.
    buffer.clear();
    midi.clear();

    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            hostPositionValid = true;
            hostPlaying = position->getIsPlaying();

            if (const auto bpm = position->getBpm())
                hostBpm = *bpm;

            if (const auto ppq = position->getPpqPosition())
            {
                hostPpq = *ppq;
                currentStep = pulseforge::TransportStep::stepIndexForPpq (*ppq);
            }
        }
        else
        {
            hostPositionValid = false;
        }
    }
    else
    {
        hostPositionValid = false;
    }
}

juce::AudioProcessorEditor* PulseForgeProcessor::createEditor()
{
    return new PulseForgeEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PulseForgeProcessor();
}
