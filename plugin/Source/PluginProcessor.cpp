#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TransportSync.h"
#include "BlockSequencer.h"
#include "PatternBankCodec.h"

#include <cmath>

namespace
{
// Parameter ids, mirrored by createParameterLayout and the state codec.
const char* const synthParamIds[2][13] = {
    { "acidA_tune", "acidA_cutoff", "acidA_resonance", "acidA_envmod", "acidA_decay",
      "acidA_accent", "acidA_attack", "acidA_sustain", "acidA_release",
      "acidA_slidetime", "acidA_accentdecay", "acidA_square", "acidA_adsr" },
    { "acidB_tune", "acidB_cutoff", "acidB_resonance", "acidB_envmod", "acidB_decay",
      "acidB_accent", "acidB_attack", "acidB_sustain", "acidB_release",
      "acidB_slidetime", "acidB_accentdecay", "acidB_square", "acidB_adsr" }
};

const char* const mixParamIds[4][5] = {
    { "mix0_pan", "mix0_send", "mix0_level", "mix0_mute", "mix0_solo" },
    { "mix1_pan", "mix1_send", "mix1_level", "mix1_mute", "mix1_solo" },
    { "mix2_pan", "mix2_send", "mix2_level", "mix2_mute", "mix2_solo" },
    { "mix3_pan", "mix3_send", "mix3_level", "mix3_mute", "mix3_solo" }
};

// Field order inside SynthParams, matching synthParamIds columns 0-10.
const char* const synthFieldJson[11] = { "tune", "cutoff", "resonance", "envMod", "decay",
                                         "accentAmount", "attack", "sustain", "release",
                                         "slideTime", "accentDecay" };

float clamp01f (double v) { return (float) juce::jlimit (0.0, 1.0, v); }
} // namespace

PulseForgeProcessor::PulseForgeProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    banks[0] = pulseforge::Pattern::demo();
    editPattern = banks[0];
    engine.setPattern (editPattern);
    updateEngineFromParameters();
}

juce::AudioProcessorValueTreeState::ParameterLayout PulseForgeProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&params] (const char* id, const juce::String& name, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id, 1), name, juce::NormalisableRange<float> (0.0f, 1.0f), def));
    };
    auto addBool = [&params] (const char* id, const juce::String& name, bool def)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (id, 1), name, def));
    };

    static const char* const synthNames[13] = { "Tune", "Cutoff", "Resonance", "Env Mod", "Decay",
                                                "Accent", "Attack", "Sustain", "Release",
                                                "Slide Time", "Accent Decay", "Square Wave", "ADSR" };

    for (int e = 0; e < 2; ++e)
    {
        // Defaults mirror SynthEngine.kt: engine A is SynthParams(), engine B
        // overrides tune/resonance/envMod/decay/accentAmount.
        const float defs[11] = { e == 0 ? .5f  : .55f,  // tune
                                 .58f,                     // cutoff
                                 e == 0 ? .42f : .46f,  // resonance
                                 e == 0 ? .62f : .72f,  // envMod
                                 e == 0 ? .54f : .61f,  // decay
                                 e == 0 ? .68f : .52f,  // accentAmount
                                 .08f, .62f, .18f, .30f, .35f };
        const juce::String title = e == 0 ? "Acid A " : "Acid B ";
        for (int k = 0; k < 11; ++k)
            addFloat (synthParamIds[e][k], title + synthNames[k], defs[k]);
        addBool (synthParamIds[e][11], title + synthNames[11], false);
        addBool (synthParamIds[e][12], title + synthNames[12], false);
    }

    static const float mixPan[4]   = { .42f, .58f, .48f, .52f };
    static const float mixSend[4]  = { .30f, .26f, .18f, .20f };
    static const float mixLevel[4] = { .78f, .70f, .82f, .72f };
    static const char* const mixNames[5] = { "Pan", "Send", "Level", "Mute", "Solo" };

    for (int ch = 0; ch < 4; ++ch)
    {
        const juce::String title = "Mixer " + juce::String (ch + 1) + " ";
        addFloat (mixParamIds[ch][0], title + mixNames[0], mixPan[ch]);
        addFloat (mixParamIds[ch][1], title + mixNames[1], mixSend[ch]);
        addFloat (mixParamIds[ch][2], title + mixNames[2], mixLevel[ch]);
        addBool  (mixParamIds[ch][3], title + mixNames[3], false);
        addBool  (mixParamIds[ch][4], title + mixNames[4], false);
    }

    addFloat ("drive", "Drive", .25f);

    return { params.begin(), params.end() };
}

float PulseForgeProcessor::paramValue (const char* id) const
{
    if (auto* v = apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

void PulseForgeProcessor::updateEngineFromParameters()
{
    for (int e = 0; e < 2; ++e)
    {
        auto& p = engine.synth[e];
        const char* const* ids = synthParamIds[e];
        p.tune         = paramValue (ids[0]);
        p.cutoff       = paramValue (ids[1]);
        p.resonance    = paramValue (ids[2]);
        p.envMod       = paramValue (ids[3]);
        p.decay        = paramValue (ids[4]);
        p.accentAmount = paramValue (ids[5]);
        p.attack       = paramValue (ids[6]);
        p.sustain      = paramValue (ids[7]);
        p.release      = paramValue (ids[8]);
        p.slideTime    = paramValue (ids[9]);
        p.accentDecay  = paramValue (ids[10]);
        p.squareWave        = paramValue (ids[11]) >= .5f;
        p.extendedEnvelope  = paramValue (ids[12]) >= .5f;
    }

    for (int ch = 0; ch < 4; ++ch)
    {
        auto& m = engine.mixer[ch];
        const char* const* ids = mixParamIds[ch];
        m.pan   = paramValue (ids[0]);
        m.send  = paramValue (ids[1]);
        m.level = paramValue (ids[2]);
        m.mute  = paramValue (ids[3]) >= .5f;
        m.solo  = paramValue (ids[4]) >= .5f;
    }

    engine.drive = paramValue ("drive");
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

    updateEngineFromParameters();

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

void PulseForgeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    const auto json = juce::JSON::toString (buildProjectVar());
    dest.setSize (json.getNumBytesAsUTF8());
    dest.copyFrom (json.toRawUTF8(), 0, json.getNumBytesAsUTF8());
}

juce::String PulseForgeProcessor::exportProjectJson()
{
    return juce::JSON::toString (buildProjectVar());
}

juce::var PulseForgeProcessor::buildProjectVar()
{
    updateEngineFromParameters();
    storeCurrentBank(); // like GrooveboxView.exportProjectJson()

    // ProjectStore.kt-compatible document. Unknown fields are ignored by the
    // Android decoder, so the plugin-only extras live under "plugin".
    auto* root = new juce::DynamicObject();
    root->setProperty ("app", "pulseforge");
    root->setProperty ("version", 1);
    const double bpm = hostBpm.load();
    root->setProperty ("bpm", bpm > 0.0 ? bpm : 126.0);
    root->setProperty ("bank", currentBank);
    root->setProperty ("banks", juce::String (pulseforge::PatternBankCodec::encodeBanks (banks)));

    juce::Array<juce::var> synthArr;
    for (int e = 0; e < 2; ++e)
    {
        const auto& p = engine.synth[e];
        auto* o = new juce::DynamicObject();
        o->setProperty ("tune", p.tune);
        o->setProperty ("cutoff", p.cutoff);
        o->setProperty ("resonance", p.resonance);
        o->setProperty ("envMod", p.envMod);
        o->setProperty ("decay", p.decay);
        o->setProperty ("accentAmount", p.accentAmount);
        o->setProperty ("squareWave", p.squareWave);
        o->setProperty ("extendedEnvelope", p.extendedEnvelope);
        o->setProperty ("attack", p.attack);
        o->setProperty ("sustain", p.sustain);
        o->setProperty ("release", p.release);
        o->setProperty ("slideTime", p.slideTime);
        o->setProperty ("accentDecay", p.accentDecay);
        synthArr.add (juce::var (o));
    }
    root->setProperty ("synth", synthArr);

    juce::Array<juce::var> mixArr;
    for (int ch = 0; ch < 4; ++ch)
    {
        const auto& m = engine.mixer[ch];
        auto* o = new juce::DynamicObject();
        o->setProperty ("pan", m.pan);
        o->setProperty ("send", m.send);
        o->setProperty ("level", m.level);
        mixArr.add (juce::var (o));
    }
    root->setProperty ("mixer", mixArr);

    auto* ext = new juce::DynamicObject();
    ext->setProperty ("drive", engine.drive);
    juce::Array<juce::var> mutes, solos;
    for (int ch = 0; ch < 4; ++ch)
    {
        mutes.add (engine.mixer[ch].mute);
        solos.add (engine.mixer[ch].solo);
    }
    ext->setProperty ("mixerMute", mutes);
    ext->setProperty ("mixerSolo", solos);
    root->setProperty ("plugin", juce::var (ext));

    return juce::var (root);
}

void PulseForgeProcessor::setParamFromState (const char* id, double value01)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (clamp01f (value01)));
}

void PulseForgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    applyProjectVar (juce::JSON::parse (juce::String::fromUTF8 ((const char*) data, sizeInBytes)));
}

bool PulseForgeProcessor::importProjectJson (const juce::String& json)
{
    return applyProjectVar (juce::JSON::parse (json));
}

bool PulseForgeProcessor::applyProjectVar (const juce::var& root)
{
    if (! root.isObject() || root.getProperty ("app", "").toString() != "pulseforge")
        return false;

    // Pattern banks first (all-or-nothing, like PatternBankStore.restore).
    const auto banksVar = root.getProperty ("banks", juce::var());
    if (banksVar.isString()
        && pulseforge::PatternBankCodec::decodeBanks (banksVar.toString().toStdString(), banks))
    {
        switchToBank ((int) root.getProperty ("bank", 0));
        engine.setPattern (editPattern);
    }

    // Synth and mixer settings: missing fields keep their current value,
    // matching ProjectStore.decode's optDouble(optBoolean) fallbacks.
    if (auto* arr = root.getProperty ("synth", juce::var()).getArray())
    {
        for (int e = 0; e < 2 && e < arr->size(); ++e)
        {
            const auto o = arr->getReference (e);
            const char* const* ids = synthParamIds[e];
            for (int k = 0; k < 11; ++k)
                setParamFromState (ids[k], (double) o.getProperty (synthFieldJson[k], paramValue (ids[k])));
            setParamFromState (ids[11], (bool) o.getProperty ("squareWave", paramValue (ids[11]) >= .5f) ? 1.0 : 0.0);
            setParamFromState (ids[12], (bool) o.getProperty ("extendedEnvelope", paramValue (ids[12]) >= .5f) ? 1.0 : 0.0);
        }
    }

    if (auto* arr = root.getProperty ("mixer", juce::var()).getArray())
    {
        for (int ch = 0; ch < 4 && ch < arr->size(); ++ch)
        {
            const auto o = arr->getReference (ch);
            const char* const* ids = mixParamIds[ch];
            setParamFromState (ids[0], (double) o.getProperty ("pan", paramValue (ids[0])));
            setParamFromState (ids[1], (double) o.getProperty ("send", paramValue (ids[1])));
            setParamFromState (ids[2], (double) o.getProperty ("level", paramValue (ids[2])));
        }
    }

    // Plugin-only extras.
    const auto ext = root.getProperty ("plugin", juce::var());
    if (ext.isObject())
    {
        setParamFromState ("drive", (double) ext.getProperty ("drive", paramValue ("drive")));
        if (auto* mutes = ext.getProperty ("mixerMute", juce::var()).getArray())
            for (int ch = 0; ch < 4 && ch < mutes->size(); ++ch)
                setParamFromState (mixParamIds[ch][3], (bool) mutes->getReference (ch) ? 1.0 : 0.0);
        if (auto* solos = ext.getProperty ("mixerSolo", juce::var()).getArray())
            for (int ch = 0; ch < 4 && ch < solos->size(); ++ch)
                setParamFromState (mixParamIds[ch][4], (bool) solos->getReference (ch) ? 1.0 : 0.0);
    }

    updateEngineFromParameters();
    return true;
}

juce::AudioProcessorEditor* PulseForgeProcessor::createEditor()
{
    return new PulseForgeEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PulseForgeProcessor();
}
