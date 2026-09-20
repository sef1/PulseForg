#include "RackPanels.h"

namespace
{
const char* const kSynthKeys[13] = { "tune", "cutoff", "resonance", "envmod", "decay", "accent",
                                     "attack", "sustain", "release", "slidetime", "accentdecay",
                                     "square", "adsr" };
const char* const kMixKeys[5] = { "pan", "send", "level", "mute", "solo" };

juce::String mixId (int ch, int col) { return "mix" + juce::String (ch) + "_" + kMixKeys[col]; }

void toggleBoolParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->getValue() < .5f ? 1.0f : 0.0f);
}

bool boolParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
{
    if (auto* v = apvts.getRawParameterValue (id))
        return v->load() >= .5f;
    return false;
}

float floatParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
{
    if (auto* v = apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

void styleCaption (juce::Label& l, juce::Component& parent)
{
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, rack::ink);
    parent.addAndMakeVisible (l);
}
} // namespace

//============================================================== HeaderPanel
HeaderPanel::HeaderPanel (PulseForgeProcessor& p) : proc (p)
{
    addAndMakeVisible (bpmDisplay);
    addAndMakeVisible (exportBtn);
    addAndMakeVisible (importBtn);
    addAndMakeVisible (strudelOutBtn);
    addAndMakeVisible (strudelInBtn);
    strudelOutBtn.onClick = [this]
    {
        juce::SystemClipboard::copyTextToClipboard (proc.exportStrudelUrl());
        strudelOutBtn.setBaseColour (rack::green);
    };
    strudelInBtn.onClick = [this]
    {
        const auto text = juce::SystemClipboard::getTextFromClipboard();
        strudelInBtn.setBaseColour (proc.importStrudel (text) ? rack::green : rack::red);
    };
    exportBtn.onClick = [this]
    {
        juce::SystemClipboard::copyTextToClipboard (proc.exportProjectJson());
        exportBtn.setBaseColour (rack::green);
    };
    importBtn.onClick = [this]
    {
        const auto text = juce::SystemClipboard::getTextFromClipboard();
        importBtn.setBaseColour (proc.importProjectJson (text) ? rack::green : rack::red);
    };
}

void HeaderPanel::paint (juce::Graphics& g)
{
    rack::paintPanel (g, getLocalBounds().toFloat());
    const float w = (float) getWidth(), h = (float) getHeight();
    g.setColour (rack::ink);
    g.setFont (rack::boldFont (h * .31f));
    g.drawText ("PULSEFORGE", (int) (w * .035f), (int) (h * .10f), (int) (w * .30f), (int) (h * .40f),
                juce::Justification::left);
    g.setColour (rack::dimText);
    g.setFont (rack::boldFont (h * .14f));
    g.drawText ("DUAL ACID / DRUM MACHINE", (int) (w * .037f), (int) (h * .55f), (int) (w * .30f), (int) (h * .30f),
                juce::Justification::left);

    juce::String transport;
    juce::Colour tc = rack::dimText;
    if (! proc.hostPositionValid.load())
        transport = "NO HOST SYNC";
    else if (proc.hostPlaying.load())
    {
        transport = "PLAYING  STEP " + juce::String (proc.currentStep.load() + 1) + " / 16";
        tc = rack::red;
    }
    else
        transport = "STOPPED";
    g.setColour (tc);
    g.setFont (rack::boldFont (h * .22f));
    g.drawText (transport, (int) (w * .26f), 0, (int) (w * .20f), (int) h, juce::Justification::centred);
    lastTransport = transport;
}

void HeaderPanel::resized()
{
    const float w = (float) getWidth(), h = (float) getHeight();
    strudelOutBtn.setBounds ((int) (w * .462f), (int) (h * .18f), (int) (w * .070f), (int) (h * .64f));
    strudelInBtn.setBounds ((int) (w * .537f), (int) (h * .18f), (int) (w * .058f), (int) (h * .64f));
    exportBtn.setBounds ((int) (w * .60f), (int) (h * .18f), (int) (w * .085f), (int) (h * .64f));
    importBtn.setBounds ((int) (w * .695f), (int) (h * .18f), (int) (w * .085f), (int) (h * .64f));
    bpmDisplay.setBounds ((int) (w * .83f), (int) (h * .10f), (int) (w * .14f), (int) (h * .80f));
}

void HeaderPanel::update()
{
    const double bpm = proc.hostBpm.load();
    bpmDisplay.setValue (bpm > 0.0 ? juce::String ((int) std::round (bpm)) : "---");
    // transport text is painted; cheap enough to refresh wholesale
    repaint();
}

//============================================================== SynthPanel
SynthPanel::SynthPanel (PulseForgeProcessor& p, int engineIndex, std::function<void (int)> onSelectTrack)
    : proc (p), eng (engineIndex), selectTrack (std::move (onSelectTrack)),
      tag (engineIndex == 0 ? "ACID ENGINE A" : "ACID ENGINE B")
{
    tag.onClick = [this] { selectTrack (eng); };
    addAndMakeVisible (tag);

    waveBtn.onClick  = [this] { toggleBoolParam (proc.apvts, synthId (11)); selectTrack (eng); };
    adsrBtn.onClick  = [this] { toggleBoolParam (proc.apvts, synthId (12)); selectTrack (eng); };
    stepEditBtn.onClick = [this] { setMode (mode == Mode::stepEdit ? Mode::knobs : Mode::stepEdit, false); selectTrack (eng); };
    accEditBtn.onClick  = [this] { setMode (Mode::knobs, ! accentEdit); selectTrack (eng); };
    advBtn.onClick      = [this] { setMode (mode == Mode::advanced ? Mode::knobs : Mode::advanced, false); selectTrack (eng); };
    for (auto* b : { &waveBtn, &adsrBtn, &stepEditBtn, &accEditBtn, &advBtn })
        addAndMakeVisible (*b);

    addChildComponent (knobRow);
    // The mode rows are full-panel containers: they must not swallow clicks
    // meant for the buttons beneath them (children still receive theirs).
    knobRow.setInterceptsMouseClicks (false, true);
    for (int i = 0; i < 6; ++i)
    {
        styleRackKnob (knobs[i]);
        knobRow.addAndMakeVisible (knobs[i]);
        styleCaption (knobLabels[i], knobRow);
    }

    addChildComponent (advRow);
    advRow.setInterceptsMouseClicks (false, true);
    for (int i = 0; i < 2; ++i)
    {
        styleRackKnob (advKnobs[i]);
        advRow.addAndMakeVisible (advKnobs[i]);
        styleCaption (advLabels[i], advRow);
        styleCaption (advValues[i], advRow);
    }
    advLabels[0].setText ("SLIDE TIME", juce::dontSendNotification);
    advLabels[1].setText ("ACC DECAY", juce::dontSendNotification);
    styleCaption (advCaption, advRow);
    advCaption.setText ("PER-ENGINE TIMING", juce::dontSendNotification);
    advCaption.setColour (juce::Label::textColourId, rack::dimText);

    addChildComponent (stepEditRow);
    stepEditRow.setInterceptsMouseClicks (false, true);
    static const char* const noteNames[7] = { "C", "D", "E", "F", "G", "A", "B" };
    for (int i = 0; i < 7; ++i)
    {
        auto* b = noteButtons.add (new RackButton (noteNames[i], rack::steel2));
        b->onClick = [this, i]
        {
            // AcidStepEditor.setPitch(): pitch classes C D E F G A B, keep octave.
            static const int pc[7] = { 0, 2, 4, 5, 7, 9, 11 };
            auto& s = stepAt (selectedStep);
            s.note = juce::jlimit (12, 96, (s.note / 12) * 12 + pc[i]);
            s.active = true;
            selectTrack (eng);
        };
        stepEditRow.addAndMakeVisible (*b);
    }
    static const char* const editNames[4] = { "OCT -", "OCT +", "ACCENT", "SLIDE" };
    for (int i = 0; i < 4; ++i)
    {
        auto* b = noteButtons.add (new RackButton (editNames[i], rack::steel2));
        b->onClick = [this, i]
        {
            auto& s = stepAt (selectedStep);
            switch (i)
            {
                case 0: s.note = juce::jlimit (12, 96, s.note - 12); s.active = true; break; // transpose
                case 1: s.note = juce::jlimit (12, 96, s.note + 12); s.active = true; break;
                case 2: s.active = true; s.accent = ! s.accent; break; // toggleAccent
                case 3: s.active = true; s.slide  = ! s.slide;  break; // toggleSlide
                default: break;
            }
            selectTrack (eng);
        };
        stepEditRow.addAndMakeVisible (*b);
    }
    styleCaption (stepReadout, stepEditRow);

    for (int i = 0; i < 16; ++i)
    {
        steps[i].onClick = [this, i]
        {
            selectedStep = i;
            selectTrack (eng);
            auto& s = stepAt (i);
            if (accentEdit)
            {
                if (s.active) s.accent = ! s.accent; // GrooveboxView acc-edit tap
            }
            else
            {
                // Step.cycleVelocity(): off -> vel1 -> vel2 -> off (accent cleared)
                if (! s.active) { s.active = true; s.velocity = 1; }
                else if (s.velocity == 1) s.velocity = 2;
                else { s.active = false; s.velocity = 1; s.accent = false; }
            }
        };
        addAndMakeVisible (steps[i]);
    }

    advAtt[0] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, synthId (9),  advKnobs[0]);
    advAtt[1] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, synthId (10), advKnobs[1]);
    attachKnobs();
    setMode (Mode::knobs, false);
}

juce::String SynthPanel::synthId (int column) const
{
    return juce::String (eng == 0 ? "acidA_" : "acidB_") + kSynthKeys[column];
}

pulseforge::AcidStep& SynthPanel::stepAt (int i)
{
    return eng == 0 ? proc.editPattern.acidA[i] : proc.editPattern.acidB[i];
}

void SynthPanel::setMode (Mode m, bool accEdit)
{
    mode = m;
    accentEdit = accEdit;
    knobRow.setVisible (mode == Mode::knobs);
    advRow.setVisible (mode == Mode::advanced);
    stepEditRow.setVisible (mode == Mode::stepEdit);
}

void SynthPanel::attachKnobs()
{
    const bool adsr = boolParam (proc.apvts, synthId (12));
    static const int normalMap[6] = { 0, 1, 2, 3, 4, 5 };
    static const int adsrMap[6]   = { 6, 4, 7, 8, 3, 5 };
    static const char* const normalNames[6] = { "TUNE", "CUTOFF", "RESO", "ENV", "DECAY", "ACCENT" };
    static const char* const adsrNames[6]   = { "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "ENV", "ACCENT" };
    for (int i = 0; i < 6; ++i)
    {
        knobAtt[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, synthId ((adsr ? adsrMap : normalMap)[i]), knobs[i]);
        knobLabels[i].setText ((adsr ? adsrNames : normalNames)[i], juce::dontSendNotification);
    }
    lastAdsr = adsr;
}

void SynthPanel::paint (juce::Graphics& g)
{
    rack::paintPanel (g, getLocalBounds().toFloat());
    const float w = (float) getWidth(), h = (float) getHeight();
    g.setFont (rack::boldFont (h * .045f));
    g.setColour (rack::red);    g.drawText ("LOW",  (int) (w * .915f), (int) (h * .50f), 60, 14, juce::Justification::left);
    g.setColour (rack::amber);  g.drawText ("HIGH", (int) (w * .915f), (int) (h * .59f), 60, 14, juce::Justification::left);
    g.setColour (rack::violet); g.drawText ("ACC",  (int) (w * .915f), (int) (h * .68f), 60, 14, juce::Justification::left);
}

void SynthPanel::resized()
{
    const float w = (float) getWidth(), h = (float) getHeight();
    auto R = [w, h] (float x0, float y0, float x1, float y1)
    {
        return juce::Rectangle<int> ((int) (x0 * w), (int) (y0 * h),
                                     (int) ((x1 - x0) * w), (int) ((y1 - y0) * h));
    };

    tag.setBounds (R (.018f, .06f, .235f, .25f));
    waveBtn.setBounds     (R (.255f, .06f, .365f, .25f));
    adsrBtn.setBounds     (R (.380f, .06f, .490f, .25f));
    stepEditBtn.setBounds (R (.505f, .06f, .650f, .25f));
    accEditBtn.setBounds  (R (.665f, .06f, .800f, .25f));
    advBtn.setBounds      (R (.815f, .06f, .940f, .25f));

    knobRow.setBounds (getLocalBounds());
    advRow.setBounds (getLocalBounds());
    stepEditRow.setBounds (getLocalBounds());

    const float ky = .49f * h, kr = .135f * h;
    for (int i = 0; i < 6; ++i)
    {
        const float kx = w * (.075f + i * .145f);
        knobs[i].setBounds ((int) (kx - kr), (int) (ky - kr), (int) (2 * kr), (int) (2 * kr));
        knobLabels[i].setBounds ((int) (kx - kr * 1.4f), (int) (ky + kr * 1.02f),
                                 (int) (kr * 2.8f), (int) (kr * .8f));
        knobLabels[i].setFont (rack::boldFont (juce::jmax (9.0f, kr * .46f)));
    }

    for (int i = 0; i < 2; ++i)
    {
        const float kx = w * (i == 0 ? .34f : .66f), r = .16f * h;
        advKnobs[i].setBounds ((int) (kx - r), (int) (ky - r), (int) (2 * r), (int) (2 * r));
        advValues[i].setBounds ((int) (kx - 60), (int) (.645f * h), 120, (int) (.08f * h));
        advValues[i].setFont (rack::boldFont (.055f * h));
        advLabels[i].setBounds ((int) (kx - 60), (int) (.72f * h), 120, (int) (.09f * h));
        advLabels[i].setFont (rack::boldFont (.05f * h));
    }
    advCaption.setBounds ((int) (w * .30f), (int) (.80f * h), (int) (w * .40f), (int) (.08f * h));
    advCaption.setFont (rack::boldFont (.045f * h));

    const float gap = .008f * w, bw = (w * .72f - gap * 6) / 7.0f, sx = .045f * w;
    for (int i = 0; i < 7; ++i)
        noteButtons[i]->setBounds ((int) (sx + i * (bw + gap)), (int) (.31f * h),
                                   (int) bw, (int) (.18f * h));
    const float cw = .18f * w;
    for (int i = 0; i < 4; ++i)
        noteButtons[7 + i]->setBounds ((int) (sx + i * (cw + gap)), (int) (.54f * h),
                                       (int) cw, (int) (.15f * h));
    stepReadout.setBounds ((int) (w * .80f), (int) (.36f * h), (int) (w * .18f), (int) (.14f * h));
    stepReadout.setFont (rack::boldFont (.06f * h));

    const float sg = .008f * w, sw = (w * .91f - sg * 15) / 16.0f, ssx = .045f * w;
    for (int i = 0; i < 16; ++i)
        steps[i].setBounds ((int) (ssx + i * (sw + sg)), (int) (.76f * h), (int) sw, (int) (.16f * h));
}

void SynthPanel::update (int selectedTrack)
{
    tag.setTagColour (selectedTrack == eng ? rack::red : rack::ink);

    const bool square = boolParam (proc.apvts, synthId (11));
    waveBtn.setButtonText (square ? "SQUARE" : "SAW");
    waveBtn.setBaseColour (square ? rack::amber : rack::steel2);

    const bool adsr = boolParam (proc.apvts, synthId (12));
    adsrBtn.setBaseColour (adsr ? rack::green : rack::steel2);
    if (adsr != lastAdsr)
        attachKnobs();

    stepEditBtn.setBaseColour (mode == Mode::stepEdit ? rack::green : rack::steel2);
    accEditBtn.setBaseColour (accentEdit ? rack::violet : rack::steel2);
    advBtn.setBaseColour (mode == Mode::advanced ? rack::green : rack::steel2);
    noteButtons[9]->setBaseColour (stepAt (selectedStep).accent ? rack::violet : rack::steel2);
    noteButtons[10]->setBaseColour (stepAt (selectedStep).slide ? rack::amber : rack::steel2);

    const bool playing = proc.hostPlaying.load();
    const int cur = proc.currentStep.load();
    for (int i = 0; i < 16; ++i)
    {
        const auto& s = stepAt (i);
        AcidStepButton::State st;
        st.active = s.active; st.accent = s.accent; st.velocity = s.velocity;
        st.current = playing && i == cur; st.index = i;
        steps[i].setState (st);
    }

    if (mode == Mode::stepEdit)
        stepReadout.setText ("S" + juce::String (selectedStep + 1)
                             + " N" + juce::String (stepAt (selectedStep).note),
                             juce::dontSendNotification);
    if (mode == Mode::advanced)
    {
        advValues[0].setText (juce::String ((int) (20 + floatParam (proc.apvts, synthId (9)) * 480)) + " MS",
                              juce::dontSendNotification);
        advValues[1].setText (juce::String ((int) (35 + floatParam (proc.apvts, synthId (10)) * 265)) + " MS",
                              juce::dontSendNotification);
    }
}

//============================================================== DrumPanel
DrumPanel::DrumPanel (PulseForgeProcessor& p, std::function<void (int)> onSelectTrack)
    : proc (p), selectTrack (std::move (onSelectTrack))
{
    tag8.onClick = [this] { selectedMachine = 0; selectTrack (2); };
    tag9.onClick = [this] { selectedMachine = 1; selectTrack (3); };
    addAndMakeVisible (tag8);
    addAndMakeVisible (tag9);
    for (int row = 0; row < 4; ++row)
        for (int i = 0; i < 16; ++i)
        {
            steps[row][i].onClick = [this, row, i]
            {
                selectTrack (2 + selectedMachine);
                auto& d = selectedMachine == 0 ? proc.editPattern.drum8 : proc.editPattern.drum9;
                switch (row)
                {
                    case 0: d.kick[i]  = ! d.kick[i];  break;
                    case 1: d.snare[i] = ! d.snare[i]; break;
                    case 2: d.hat[i]   = ! d.hat[i];   break;
                    default: d.clap[i] = ! d.clap[i];  break;
                }
            };
            addAndMakeVisible (steps[row][i]);
        }
}

void DrumPanel::paint (juce::Graphics& g)
{
    rack::paintPanel (g, getLocalBounds().toFloat());
    const float w = (float) getWidth(), h = (float) getHeight();
    g.setColour (rack::dimText);
    g.setFont (rack::boldFont (h * .075f));
    g.drawText ("16-STEP RHYTHM", (int) (w * .60f), (int) (h * .05f), (int) (w * .375f), (int) (h * .14f),
                juce::Justification::right);
    static const char* const names[4] = { "BD", "SD", "CH", "CP" };
    g.setColour (rack::ink);
    g.setFont (rack::boldFont (h * .075f));
    for (int row = 0; row < 4; ++row)
        g.drawText (names[row], (int) (w * .030f), (int) (h * (.27f + row * .17f)),
                    (int) (w * .06f), (int) (h * .12f), juce::Justification::left);
}

void DrumPanel::resized()
{
    const float w = (float) getWidth(), h = (float) getHeight();
    tag8.setBounds ((int) (w * .018f), (int) (h * .045f), (int) (w * .172f), (int) (h * .145f));
    tag9.setBounds ((int) (w * .205f), (int) (h * .045f), (int) (w * .172f), (int) (h * .145f));
    const float sg = .006f * w, sw = (w * .865f - sg * 15) / 16.0f, sx = .10f * w;
    for (int row = 0; row < 4; ++row)
        for (int i = 0; i < 16; ++i)
            steps[row][i].setBounds ((int) (sx + i * (sw + sg)), (int) (h * (.27f + row * .17f)),
                                     (int) sw, (int) (h * .12f));
}

void DrumPanel::update (int selectedTrack)
{
    tag8.setTagColour (selectedTrack == 2 ? rack::amber : rack::ink);
    tag9.setTagColour (selectedTrack == 3 ? rack::amber : rack::ink);
    const auto& d = selectedMachine == 0 ? proc.editPattern.drum8 : proc.editPattern.drum9;
    const bool playing = proc.hostPlaying.load();
    const int cur = proc.currentStep.load();
    for (int row = 0; row < 4; ++row)
        for (int i = 0; i < 16; ++i)
        {
            DrumStepButton::State st;
            st.active = row == 0 ? d.kick[i] : row == 1 ? d.snare[i] : row == 2 ? d.hat[i] : d.clap[i];
            st.current = playing && i == cur;
            st.amberLane = row == 0;
            st.index = i;
            steps[row][i].setState (st);
        }
}

//============================================================== MixerPanel
MixerPanel::MixerPanel (PulseForgeProcessor& p, std::function<void (int)> onSelectTrack)
    : proc (p), selectTrack (std::move (onSelectTrack))
{
    static const char* const channelNames[4] = { "ACID A", "ACID B", "DRUM 8", "DRUM 9" };
    for (int ch = 0; ch < 4; ++ch)
    {
        styleCaption (names[ch], *this);
        names[ch].setText (channelNames[ch], juce::dontSendNotification);
        styleRackKnob (panKnobs[ch]);
        styleRackKnob (sendKnobs[ch]);
        addAndMakeVisible (panKnobs[ch]);
        addAndMakeVisible (sendKnobs[ch]);
        styleCaption (panLabels[ch], *this);
        styleCaption (sendLabels[ch], *this);
        panLabels[ch].setText ("PAN", juce::dontSendNotification);
        sendLabels[ch].setText ("SEND", juce::dontSendNotification);
        faders[ch].setSliderStyle (juce::Slider::LinearVertical);
        faders[ch].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (faders[ch]);
        addAndMakeVisible (muteBtns[ch]);
        addAndMakeVisible (soloBtns[ch]);
        addAndMakeVisible (lamps[ch]);

        panAtt[ch]   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, mixId (ch, 0), panKnobs[ch]);
        sendAtt[ch]  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, mixId (ch, 1), sendKnobs[ch]);
        levelAtt[ch] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, mixId (ch, 2), faders[ch]);

        muteBtns[ch].onClick = [this, ch] { toggleBoolParam (proc.apvts, mixId (ch, 3)); };
        soloBtns[ch].onClick = [this, ch] { toggleBoolParam (proc.apvts, mixId (ch, 4)); };
    }
}

void MixerPanel::paint (juce::Graphics& g)
{
    rack::paintPanel (g, getLocalBounds().toFloat());
    const float h = (float) getHeight();
    g.setColour (rack::ink);
    g.setFont (rack::boldFont (h * .09f));
    g.drawText ("MIXER", (int) (getWidth() * .025f), (int) (h * .06f), 100, (int) (h * .14f),
                juce::Justification::left);
}

void MixerPanel::resized()
{
    const float w = (float) getWidth(), h = (float) getHeight();
    for (int ch = 0; ch < 4; ++ch)
    {
        const float x = w * (.18f + ch * .205f);
        names[ch].setBounds ((int) (x - w * .09f), (int) (h * .06f), (int) (w * .18f), (int) (h * .11f));
        names[ch].setFont (rack::boldFont (h * .066f));
        const float kr = h * .10f;
        const float panX = x - w * .037f, sendX = x + w * .050f, ky = h * .34f;
        panKnobs[ch].setBounds ((int) (panX - kr), (int) (ky - kr), (int) (2 * kr), (int) (2 * kr));
        sendKnobs[ch].setBounds ((int) (sendX - kr), (int) (ky - kr), (int) (2 * kr), (int) (2 * kr));
        panLabels[ch].setBounds ((int) (panX - 24), (int) (ky + kr * 1.0f), 48, (int) (h * .09f));
        sendLabels[ch].setBounds ((int) (sendX - 24), (int) (ky + kr * 1.0f), 48, (int) (h * .09f));
        panLabels[ch].setFont (rack::boldFont (h * .055f));
        sendLabels[ch].setFont (rack::boldFont (h * .055f));
        faders[ch].setBounds ((int) (x - 22), (int) (h * .50f), 44, (int) (h * .38f));
        lamps[ch].setBounds ((int) (x - w * .062f - 7), (int) (h * .80f - 7), 14, 14);
        muteBtns[ch].setBounds ((int) (x + w * .066f), (int) (h * .49f), (int) (w * .056f), (int) (h * .13f));
        soloBtns[ch].setBounds ((int) (x + w * .066f), (int) (h * .65f), (int) (w * .056f), (int) (h * .13f));
    }
}

void MixerPanel::update (int selectedTrack)
{
    for (int ch = 0; ch < 4; ++ch)
    {
        muteBtns[ch].setBaseColour (boolParam (proc.apvts, mixId (ch, 3)) ? rack::red : rack::steel2);
        soloBtns[ch].setBaseColour (boolParam (proc.apvts, mixId (ch, 4)) ? rack::green : rack::steel2);
        lamps[ch].setOn (selectedTrack == ch);
    }
}

//============================================================== FooterPanel
FooterPanel::FooterPanel (PulseForgeProcessor& p) : proc (p)
{
    addAndMakeVisible (memDisplay);
    for (int i = 0; i < 8; ++i)
    {
        bankBtns[i].onClick = [this, i] { proc.switchToBank (i); };
        addAndMakeVisible (bankBtns[i]);
    }
    clearBtn.onClick = [this] { proc.editPattern.clear(); };
    storeBtn.onClick = [this] { proc.storeCurrentBank(); };
    addAndMakeVisible (clearBtn);
    addAndMakeVisible (storeBtn);
}

void FooterPanel::paint (juce::Graphics& g)
{
    rack::paintPanel (g, getLocalBounds().toFloat());
    const float h = (float) getHeight();
    g.setColour (rack::ink);
    g.setFont (rack::boldFont (h * .095f));
    g.drawText ("PATTERN BANK", (int) (getWidth() * .025f), (int) (h * .06f), 200, (int) (h * .16f),
                juce::Justification::left);
}

void FooterPanel::resized()
{
    const float w = (float) getWidth(), h = (float) getHeight();
    memDisplay.setBounds ((int) (w * .81f), (int) (h * .055f), (int) (w * .165f), (int) (h * .26f));
    const float g2 = .014f * w, bw = (w * .91f - g2 * 7) / 8.0f, x = .045f * w;
    for (int i = 0; i < 8; ++i)
        bankBtns[i].setBounds ((int) (x + i * (bw + g2)), (int) (h * .34f), (int) bw, (int) (h * .26f));
    clearBtn.setBounds ((int) (w * .045f), (int) (h * .70f), (int) (w * .255f), (int) (h * .24f));
    storeBtn.setBounds ((int) (w * .325f), (int) (h * .70f), (int) (w * .265f), (int) (h * .24f));
}

void FooterPanel::update()
{
    memDisplay.setValue (juce::String (proc.currentBank + 1));
    for (int i = 0; i < 8; ++i)
        bankBtns[i].setBaseColour (i == proc.currentBank ? rack::red : rack::steel2);
}
