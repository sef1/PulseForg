#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

// Visual language ported 1:1 from GrooveboxView.kt (colors, panel gradients,
// screws, steel buttons, knob pointer geometry, LED displays, step colors).

namespace rack
{
const juce::Colour darkBg    { 0xff0f1112 };
const juce::Colour panelTop  { 0xffe0e3e1 };
const juce::Colour panelBot  { 0xff919797 };
const juce::Colour panelEdge { 0xff4a4f4f };
const juce::Colour screwDark { 0xff565b5b };
const juce::Colour screwHi   { 0xffd2d6d4 };
const juce::Colour ink       { 0xff191c1d };
const juce::Colour red       { 0xffee372b };
const juce::Colour led       { 0xffff4230 };
const juce::Colour amber     { 0xffffad1f };
const juce::Colour green     { 0xff64cd75 };
const juce::Colour violet    { 0xffb153ff };
const juce::Colour steel2    { 0xff9ca3a3 };
const juce::Colour stepOff   { 0xff4d5251 };
const juce::Colour btnBorder { 0xff484c4c };
const juce::Colour dispBg    { 0xff140e0d };
const juce::Colour dimText   { 0xff55595a };
const juce::Colour knobOuter { 0xff333737 };
const juce::Colour knobInner { 0xff686d6c };
const juce::Colour faderTrack{ 0xff3b3f3f };
const juce::Colour faderThumb{ 0xffe8eae7 };
const juce::Colour lampOff   { 0xff555c5b };

inline void paintPanel (juce::Graphics& g, juce::Rectangle<float> b)
{
    g.setGradientFill (juce::ColourGradient (panelTop, b.getX(), b.getY(),
                                             panelBot, b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 2.0f);
    auto screw = [&g] (float x, float y)
    {
        g.setColour (screwDark); g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        g.setColour (screwHi);   g.drawLine (x - 2.2f, y, x + 2.2f, y, 1.3f);
    };
    screw (b.getX() + 9, b.getY() + 9);
    screw (b.getRight() - 9, b.getY() + 9);
    screw (b.getX() + 9, b.getBottom() - 9);
    screw (b.getRight() - 9, b.getBottom() - 9);
}

inline juce::Font boldFont (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
} // namespace rack

class RackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RackLookAndFeel() { setDefaultLookAndFeel (this); }

    // GrooveboxView knob(): dark body, lighter inner cap, white pointer at
    // 135deg + 270deg * value (canvas coords), label drawn by the panel.
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override
    {
        const float cx = x + w * .5f, cy = y + h * .5f;
        const float r = juce::jmin ((float) w, (float) h) * .5f - 2.0f;
        if (r <= 0.0f) return;
        g.setColour (rack::knobOuter); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
        const float ri = r * .73f;
        g.setColour (rack::knobInner); g.fillEllipse (cx - ri, cy - ri, ri * 2, ri * 2);
        // JUCE angles run clockwise from 12 o'clock.
        const float angle = startAngle + pos * (endAngle - startAngle);
        g.setColour (juce::Colours::white);
        g.drawLine (cx, cy,
                    cx + std::sin (angle) * r * .64f,
                    cy - std::cos (angle) * r * .64f,
                    juce::jmax (2.0f, r * .09f));
    }

    // GrooveboxView fader(): thick track, white thumb with ink center line.
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float, float,
                           const juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s);
            return;
        }
        const float cx = x + w * .5f;
        g.setColour (rack::faderTrack);
        g.drawLine (cx, (float) y + 8.0f, cx, (float) (y + h) - 8.0f, 7.0f);
        g.setColour (rack::faderThumb);
        g.fillRoundedRectangle (cx - 18.0f, pos - 8.0f, 36.0f, 16.0f, 3.0f);
        g.setColour (rack::ink);
        g.drawLine (cx - 15.0f, pos, cx + 15.0f, pos, 2.0f);
    }
};

// GrooveboxView button(): dark border, inset colored face, bold centered text.
class RackButton : public juce::Button
{
public:
    RackButton (juce::String text, juce::Colour base) : juce::Button (text), baseColour (base) {}

    void setBaseColour (juce::Colour c) { baseColour = c; repaint(); }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (rack::btnBorder);
        g.fillRoundedRectangle (b, 5.0f);
        auto inner = b.reduced (3.0f, 3.0f);
        inner.setBottom (b.getBottom() - 4.0f);
        juce::Colour c = baseColour;
        if (down) c = c.darker (0.25f);
        else if (highlighted) c = c.brighter (0.08f);
        g.setColour (c);
        g.fillRoundedRectangle (inner, 4.0f);
        const bool lightText = baseColour == rack::red || baseColour == rack::green
                            || baseColour == rack::violet;
        g.setColour (lightText ? juce::Colours::white : rack::ink);
        g.setFont (rack::boldFont (juce::jmin (13.0f, inner.getHeight() * .42f)));
        g.drawText (getButtonText(), inner, juce::Justification::centred);
    }

private:
    juce::Colour baseColour;
};

// GrooveboxView tag(): colored rounded title bar with white bold text.
class RackTag : public juce::Component
{
public:
    explicit RackTag (juce::String text) : t (text) {}
    std::function<void()> onClick;

    void setTagColour (juce::Colour c) { col = c; repaint(); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
        g.setColour (juce::Colours::white);
        g.setFont (rack::boldFont (getHeight() * .36f));
        g.drawText (t, getLocalBounds(), juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }

private:
    juce::String t;
    juce::Colour col { rack::ink };
};

// GrooveboxView display(): dark inset box, red LED value, caption below.
class RackDisplay : public juce::Component
{
public:
    explicit RackDisplay (juce::String label) : labelText (label) {}

    void setValue (const juce::String& v)
    {
        if (valueText != v) { valueText = v; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto box = b.withHeight (b.getHeight() * .76f);
        g.setColour (rack::dispBg);
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (rack::led);
        g.setFont (rack::boldFont (box.getHeight() * .58f));
        g.drawText (valueText, box, juce::Justification::centred);
        g.setColour (rack::ink);
        g.setFont (rack::boldFont (b.getHeight() * .16f));
        g.drawText (labelText, b.withTrimmedTop (box.getHeight()), juce::Justification::centred);
    }

private:
    juce::String labelText, valueText { "-" };
};

// GrooveboxView acidStep(): velocity color, violet accent ring + dot, number.
class AcidStepButton : public juce::Component
{
public:
    std::function<void()> onClick;

    struct State
    {
        bool active = false, accent = false, current = false;
        int velocity = 1, index = 0;
        bool operator== (const State& o) const
        {
            return active == o.active && accent == o.accent && current == o.current
                && velocity == o.velocity && index == o.index;
        }
    };

    void setState (const State& s)
    {
        if (! (s == state)) { state = s; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        const juce::Colour c = state.current ? rack::green
                             : ! state.active ? rack::stepOff
                             : state.velocity == 2 ? rack::amber : rack::red;
        g.setColour (c);
        g.fillRoundedRectangle (b, 3.0f);
        if (state.accent)
        {
            g.setColour (rack::violet);
            g.drawRoundedRectangle (b.reduced (1.0f), 3.0f, juce::jmax (2.5f, b.getWidth() * .10f));
            const float d = juce::jmax (3.0f, b.getHeight() * .16f);
            g.fillEllipse (b.getX() + b.getWidth() * .72f, b.getY() + b.getHeight() * .10f, d, d);
        }
        g.setColour (state.active || state.current ? juce::Colours::white : juce::Colours::lightgrey);
        g.setFont (rack::boldFont (b.getHeight() * .40f));
        g.drawText (juce::String (state.index + 1), b, juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }

private:
    State state;
};

// GrooveboxView step() for drums: lane color when active, green when current.
class DrumStepButton : public juce::Component
{
public:
    std::function<void()> onClick;

    struct State
    {
        bool active = false, current = false, amberLane = false;
        int index = 0;
        bool operator== (const State& o) const
        {
            return active == o.active && current == o.current
                && amberLane == o.amberLane && index == o.index;
        }
    };

    void setState (const State& s)
    {
        if (! (s == state)) { state = s; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        const juce::Colour c = state.current ? rack::green
                             : state.active ? (state.amberLane ? rack::amber : rack::red)
                             : rack::stepOff;
        g.setColour (c);
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (state.active || state.current ? juce::Colours::white : juce::Colours::lightgrey);
        g.setFont (rack::boldFont (b.getHeight() * .40f));
        g.drawText (juce::String (state.index + 1), b, juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }

private:
    State state;
};

// GrooveboxView lamp(): small status circle.
class RackLamp : public juce::Component
{
public:
    void setOn (bool o)
    {
        if (o != on) { on = o; repaint(); }
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (on ? rack::green : rack::lampOff);
        const float d = juce::jmin ((float) getWidth(), (float) getHeight());
        g.fillEllipse (getLocalBounds().toFloat().withSizeKeepingCentre (d, d));
    }
private:
    bool on = false;
};

inline void styleRackKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    // 1.25pi..2.75pi == GrooveboxView's 135deg..405deg sweep.
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                           juce::MathConstants<float>::pi * 2.75f, true);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
}
