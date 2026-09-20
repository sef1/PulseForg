#pragma once

#include "RackUI.h"
#include "../PluginProcessor.h"

/**
 * M3b rack panels: JUCE port of GrooveboxView.kt's sections. Internal
 * layout and interactions follow the Android view 1:1; the two-column
 * arrangement lives in the editor (DAW windows are landscape, phones are
 * not). Host transport replaces the app's local PLAY/STOP button.
 */

class HeaderPanel : public juce::Component
{
public:
    explicit HeaderPanel (PulseForgeProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update(); // 30 Hz from the editor timer

private:
    PulseForgeProcessor& proc;
    RackDisplay bpmDisplay { "BPM" };
    RackButton exportBtn { "EXPORT", rack::steel2 }, importBtn { "IMPORT", rack::steel2 };
    RackButton strudelOutBtn { "STRD OUT", rack::steel2 }, strudelInBtn { "STRD IN", rack::steel2 };
    juce::String lastTransport;
};

class SynthPanel : public juce::Component
{
public:
    SynthPanel (PulseForgeProcessor&, int engineIndex, std::function<void (int)> onSelectTrack);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update (int selectedTrack); // 30 Hz

private:
    enum class Mode { knobs, stepEdit, advanced };

    void attachKnobs();
    void setMode (Mode, bool accentEdit);
    pulseforge::AcidStep& stepAt (int i);
    juce::String synthId (int column) const;

    PulseForgeProcessor& proc;
    const int eng;
    std::function<void (int)> selectTrack;

    RackTag tag;
    RackButton waveBtn { "SAW", rack::steel2 };
    RackButton adsrBtn { "ADSR", rack::steel2 };
    RackButton stepEditBtn { "STEP EDIT", rack::steel2 };
    RackButton accEditBtn { "ACC EDIT", rack::steel2 };
    RackButton advBtn { "ADV", rack::steel2 };

    juce::Component knobRow, advRow, stepEditRow;
    juce::Slider knobs[6];
    juce::Label knobLabels[6];
    juce::Slider advKnobs[2];
    juce::Label advLabels[2], advValues[2], advCaption;
    juce::OwnedArray<RackButton> noteButtons; // 7 pitch keys + 4 edit buttons
    juce::Label stepReadout;
    AcidStepButton steps[16];

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtt[6];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> advAtt[2];

    Mode mode = Mode::knobs;
    bool accentEdit = false;
    bool lastAdsr = false;
    int selectedStep = 0;
};

class DrumPanel : public juce::Component
{
public:
    DrumPanel (PulseForgeProcessor&, std::function<void (int)> onSelectTrack);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update (int selectedTrack);

private:
    PulseForgeProcessor& proc;
    std::function<void (int)> selectTrack;
    RackTag tag8 { "DRUM 8" }, tag9 { "DRUM 9" };
    DrumStepButton steps[4][16];
    int selectedMachine = 0; // 0 = drum8, 1 = drum9 (track 2 / 3)
};

class MixerPanel : public juce::Component
{
public:
    MixerPanel (PulseForgeProcessor&, std::function<void (int)> onSelectTrack);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update (int selectedTrack);

private:
    PulseForgeProcessor& proc;
    std::function<void (int)> selectTrack;
    juce::Label names[4];
    juce::Slider panKnobs[4], sendKnobs[4], faders[4];
    juce::Label panLabels[4], sendLabels[4];
    RackButton muteBtns[4] { { "M", rack::steel2 }, { "M", rack::steel2 },
                             { "M", rack::steel2 }, { "M", rack::steel2 } };
    RackButton soloBtns[4] { { "S", rack::steel2 }, { "S", rack::steel2 },
                             { "S", rack::steel2 }, { "S", rack::steel2 } };
    RackLamp lamps[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt[4], sendAtt[4], levelAtt[4];
};

class FooterPanel : public juce::Component
{
public:
    explicit FooterPanel (PulseForgeProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update();

private:
    PulseForgeProcessor& proc;
    RackDisplay memDisplay { "MEM" };
    RackButton bankBtns[8] { { "A", rack::steel2 }, { "B", rack::steel2 }, { "C", rack::steel2 },
                             { "D", rack::steel2 }, { "E", rack::steel2 }, { "F", rack::steel2 },
                             { "G", rack::steel2 }, { "H", rack::steel2 } };
    RackButton clearBtn { "CLEAR", rack::steel2 }, storeBtn { "STORE", rack::steel2 };
};
