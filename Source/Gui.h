#pragma once

// Éléments d'interface du Prophorcel : aspect de face avant (alu brossé, boutons à collerette,
// curseurs à capuchon, voyants lumineux), joystick vectoriel, oscilloscope, molettes.

#include "PluginProcessor.h"

namespace gui
{
using namespace juce;

constexpr int kDesignW = 1500;
constexpr int kDesignH = 800;

inline const Colour kInk (0xff1d2024), kInk2 (0xff444952), kReadout (0xff6a3f08), kAmber (0xffffb54a);
inline const Colour kOsc[4] = { Colour (0xff4cc9f0), Colour (0xfff472b6), Colour (0xfffacc15), Colour (0xff4ade80) };
inline const Colour kModVec (0xffd98a45), kModOsc (0xff3bb3aa), kModFlt (0xffd4574c), kModEnv (0xffe0b232),
                    kModLfo (0xff9a83e0), kModVoi (0xff5b95d6), kModFx (0xff62b476);

inline Font uiFont (float h, bool bold = true)
{
    return Font (FontOptions (h, bold ? Font::bold : Font::plain).withKerningFactor (0.06f));
}
inline Font monoFont (float h) { return Font (FontOptions (Font::getDefaultMonospacedFontName(), h, Font::plain)); }

inline String fHz (double v) { return v >= 1000.0 ? String (v / 1000.0, 1) + " kHz" : String (roundToInt (v)) + " Hz"; }
inline String fS (double v) { return v < 1.0 ? String (roundToInt (v * 1000.0)) + " ms" : String (v, 2) + " s"; }
inline String fPct (double v) { return String (roundToInt (v * 100.0)) + " %"; }
inline String fDb (double v) { return String (v, 1) + " dB"; }
inline String fOct (double v) { return String (v, 1) + " oct"; }
inline String fCt (double v) { return String (roundToInt (v)) + " ct"; }
inline String fSigned (double v) { const int i = roundToInt (v); return (i > 0 ? "+" : "") + String (i); }
inline String fSignedCt (double v) { return fSigned (v) + " ct"; }
inline String fHzLow (double v) { return String (v, 2) + " Hz"; }
inline String fHz1 (double v) { return String (v, 1) + " Hz"; }

//==============================================================================
class PanelLookAndFeel : public LookAndFeel_V4
{
public:
    PanelLookAndFeel()
    {
        setColour (PopupMenu::backgroundColourId, Colour (0xff1b1d20));
        setColour (PopupMenu::textColourId, Colour (0xffece6d8));
        setColour (PopupMenu::highlightedBackgroundColourId, kAmber);
        setColour (PopupMenu::highlightedTextColourId, Colour (0xff1a0e06));
    }

    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float pos, float startA, float endA, Slider& s) override
    {
        const auto b = Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float angle = startA + pos * (endA - startA);
        auto pt = [&] (float a, float r) { return Point<float> (c.x + r * std::sin (a), c.y - r * std::cos (a)); };

        g.setColour (kInk2.withAlpha (0.8f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startA + (endA - startA) * (float) i / 10.f;
            g.drawLine (Line<float> (pt (a, (i % 5 == 0) ? R * 0.80f : R * 0.86f), pt (a, R * 0.97f)), 1.4f);
        }

        const float rc = R * 0.70f;
        Path capPath;
        capPath.addEllipse (c.x - rc, c.y - rc, rc * 2.f, rc * 2.f);
        DropShadow (Colours::black.withAlpha (0.5f), jmax (2, (int) (R * 0.25f)), { 0, jmax (1, (int) (R * 0.1f)) }).drawForPath (g, capPath);
        g.setColour (Colour (0xff17191c));
        g.fillPath (capPath);

        // collerette moletée : elle tourne avec le bouton
        g.setColour (Colour (0xff3a3e44));
        const int teeth = 30;
        for (int i = 0; i < teeth; ++i)
        {
            const float a = angle + (float) i * MathConstants<float>::twoPi / (float) teeth;
            g.drawLine (Line<float> (pt (a, rc * 0.84f), pt (a, rc * 0.99f)), 1.7f);
        }

        const Colour mod = s.findColour (Slider::rotarySliderFillColourId);
        const float rt = rc * 0.78f;
        ColourGradient top (mod.interpolatedWith (Colour (0xff33363b), 0.45f), c.x, c.y,
                            mod.interpolatedWith (Colour (0xff17191c), 0.70f), c.x + rt, c.y, true);
        g.setGradientFill (top);
        g.fillEllipse (c.x - rt, c.y - rt, rt * 2.f, rt * 2.f);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawEllipse (c.x - rt, c.y - rt, rt * 2.f, rt * 2.f, 1.f);

        Path ptr;
        ptr.startNewSubPath (pt (angle, rt * 0.30f));
        ptr.lineTo (pt (angle, rc * 0.97f));
        g.setColour (Colour (0xfff6f0e2));
        g.strokePath (ptr, PathStrokeType (jmax (2.f, R * 0.07f), PathStrokeType::curved, PathStrokeType::rounded));

        ColourGradient gloss (Colours::white.withAlpha (0.38f), c.x - rc * 0.36f, c.y - rc * 0.46f,
                              Colours::white.withAlpha (0.f), c.x - rc * 0.36f + rc * 0.95f, c.y - rc * 0.46f, true);
        g.setGradientFill (gloss);
        g.fillEllipse (c.x - rc, c.y - rc, rc * 2.f, rc * 2.f);
    }

    int getSliderThumbRadius (Slider&) override { return 11; }

    void drawLinearSlider (Graphics& g, int x, int y, int width, int height, float sliderPos, float, float,
                           Slider::SliderStyle, Slider& s) override
    {
        const float cx = (float) x + (float) width * 0.5f;
        const float top = (float) y + 10.f, bottom = (float) (y + height) - 10.f;

        g.setColour (Colour (0xff101113));
        g.fillRoundedRectangle (cx - 3.f, top, 6.f, bottom - top, 3.f);
        g.setColour (Colours::white.withAlpha (0.55f));
        g.drawLine (cx - 3.f, bottom + 0.5f, cx + 3.f, bottom + 0.5f, 1.f);

        g.setColour (kInk2.withAlpha (0.75f));
        for (float ty = top; ty <= bottom + 0.5f; ty += 11.f)
        {
            g.drawLine ((float) x, ty, (float) x + 5.f, ty, 1.f);
            g.drawLine ((float) (x + width) - 5.f, ty, (float) (x + width), ty, 1.f);
        }

        const float capY = jlimit (top + 1.f, bottom - 1.f, sliderPos);
        const auto cap = Rectangle<float> ((float) x + 3.f, capY - 11.f, (float) width - 6.f, 22.f);
        Path p;
        p.addRoundedRectangle (cap, 3.f);
        DropShadow (Colours::black.withAlpha (0.5f), 4, { 0, 2 }).drawForPath (g, p);
        ColourGradient grad (Colour (0xff80858d), cap.getX(), cap.getY(), Colour (0xff50545b), cap.getX(), cap.getBottom(), false);
        grad.addColour (0.46, Colour (0xff494d54));
        grad.addColour (0.54, Colour (0xff23262a));
        g.setGradientFill (grad);
        g.fillPath (p);
        g.setColour (Colours::white.withAlpha (0.35f));
        g.drawLine (cap.getX() + 2.f, cap.getY() + 0.5f, cap.getRight() - 2.f, cap.getY() + 0.5f, 1.f);
        g.setColour (Colour (0xfff6f0e2));
        g.fillRect (Rectangle<float> (cap.getX() + 4.f, cap.getCentreY() - 1.f, cap.getWidth() - 8.f, 2.f));
        (void) s;
    }

    Font getComboBoxFont (ComboBox&) override { return monoFont (12.f); }

    void drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box) override
    {
        const auto r = Rectangle<float> (0.f, 0.f, (float) width, (float) height);
        g.setColour (box.findColour (ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);
        Path arrow;
        const float ax = (float) width - 14.f, ay = (float) height * 0.5f;
        arrow.addTriangle (ax, ay - 2.f, ax + 8.f, ay - 2.f, ax + 4.f, ay + 3.f);
        g.setColour (box.findColour (ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    void positionComboBoxText (ComboBox& box, Label& label) override
    {
        label.setBounds (8, 1, box.getWidth() - 28, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawButtonBackground (Graphics& g, Button& b, const Colour&, bool, bool down) override
    {
        const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        ColourGradient grad (Colour (0xff4a4e55), 0.f, r.getY(), Colour (0xff1f2124), 0.f, r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r.translated (0.f, down ? 1.f : 0.f), 4.f);
        g.setColour (Colours::white.withAlpha (0.18f));
        g.drawLine (r.getX() + 3.f, r.getY() + 0.5f, r.getRight() - 3.f, r.getY() + 0.5f, 1.f);
    }

    Font getTextButtonFont (TextButton&, int) override { return uiFont (12.f); }
};

//==============================================================================
// Clavier : touches ivoire et noires brillantes.
class PanelKeyboard : public MidiKeyboardComponent
{
public:
    PanelKeyboard (MidiKeyboardState& state) : MidiKeyboardComponent (state, MidiKeyboardComponent::horizontalKeyboard) {}

protected:
    void drawWhiteNote (int, Graphics& g, Rectangle<float> area, bool isDown, bool, Colour, Colour) override
    {
        Path p;
        p.addRoundedRectangle (area.getX(), area.getY(), area.getWidth(), area.getHeight(), 4.f, 4.f, false, false, true, true);
        ColourGradient grad = isDown
            ? ColourGradient (Colour (0xffffdc9a), 0.f, area.getY(), Colour (0xffc58b3a), 0.f, area.getBottom(), false)
            : ColourGradient (Colour (0xfffcf9f0), 0.f, area.getY(), Colour (0xffcfc7b4), 0.f, area.getBottom(), false);
        grad.addColour (0.82, isDown ? Colour (0xfff0b660) : Colour (0xffede6d5));
        g.setGradientFill (grad);
        g.fillPath (p);
        g.setColour (Colour (0xffa9a290));
        g.drawLine (area.getRight() - 0.5f, area.getY(), area.getRight() - 0.5f, area.getBottom(), 1.f);
    }

    void drawBlackNote (int, Graphics& g, Rectangle<float> area, bool isDown, bool, Colour) override
    {
        Path p;
        p.addRoundedRectangle (area.getX(), area.getY(), area.getWidth(), area.getHeight(), 3.f, 3.f, false, false, true, true);
        DropShadow (Colours::black.withAlpha (0.55f), 5, { 0, 4 }).drawForPath (g, p);
        ColourGradient grad (isDown ? Colour (0xff6a3a14) : Colour (0xff17181b), area.getX(), 0.f,
                             isDown ? Colour (0xff6a3a14) : Colour (0xff0b0c0d), area.getRight(), 0.f, false);
        grad.addColour (0.35, isDown ? Colour (0xffb5651f) : Colour (0xff3a3d42));
        grad.addColour (0.65, isDown ? Colour (0xffb5651f) : Colour (0xff17181b));
        g.setGradientFill (grad);
        g.fillPath (p);
        g.setColour (Colours::black);
        g.strokePath (p, PathStrokeType (1.f));
    }
};

//==============================================================================
// Bouton rotatif avec son nom au-dessus et sa valeur dessous.
class Knob : public Component
{
public:
    Knob (AudioProcessorValueTreeState& apvts, const String& id, const String& labelText, std::function<String (double)> fmt,
          Colour mod, int diameter)
        : label (labelText), d (diameter)
    {
        slider.setSliderStyle (Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (170);
        slider.setColour (Slider::rotarySliderFillColourId, mod);
        addAndMakeVisible (slider);
        attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, id, slider);
        slider.textFromValueFunction = fmt;   // après l'attachement, qui installe son propre format
        slider.onValueChange = [this] { repaint(); };
    }

    void resized() override
    {
        slider.setBounds ((getWidth() - d) / 2, 14, d, d);
    }

    void paint (Graphics& g) override
    {
        g.setColour (kInk2);
        g.setFont (uiFont (11.f));
        g.drawText (label.toUpperCase(), 0, 0, getWidth(), 13, Justification::centred, false);
        g.setColour (kReadout);
        g.setFont (monoFont (10.5f));
        g.drawText (slider.getTextFromValue (slider.getValue()), 0, 14 + d + 3, getWidth(), 12, Justification::centred, false);
    }

    static int heightFor (int diameter) { return 14 + diameter + 3 + 12; }

private:
    Slider slider;
    String label;
    int d;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attach;
};

// Curseur vertical à capuchon (enveloppes).
class Fader : public Component
{
public:
    Fader (AudioProcessorValueTreeState& apvts, const String& id, const String& labelText, std::function<String (double)> fmt)
        : label (labelText)
    {
        slider.setSliderStyle (Slider::LinearVertical);
        slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (slider);
        attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, id, slider);
        slider.textFromValueFunction = fmt;
        slider.onValueChange = [this] { repaint(); };
    }

    void resized() override { slider.setBounds ((getWidth() - 34) / 2, 14, 34, 100); }

    void paint (Graphics& g) override
    {
        g.setColour (kInk2);
        g.setFont (uiFont (11.f));
        g.drawText (label.toUpperCase(), 0, 0, getWidth(), 13, Justification::centred, false);
        g.setColour (kReadout);
        g.setFont (monoFont (10.f));
        g.drawText (slider.getTextFromValue (slider.getValue()), 0, 14 + 100 + 3, getWidth(), 12, Justification::centred, false);
    }

    static int height() { return 14 + 100 + 3 + 12; }

private:
    Slider slider;
    String label;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attach;
};

// Voyants-poussoirs : un seul (bascule) ou un groupe exclusif (paramètre à choix).
class LedGroup : public Component
{
public:
    LedGroup (AudioProcessorValueTreeState& apvts, const String& id, const String& labelText,
              std::vector<String> names, Colour colour, bool toggleMode)
        : label (labelText), items (std::move (names)), led (colour), toggle (toggleMode)
    {
        auto* param = apvts.getParameter (id);
        jassert (param != nullptr);
        attach = std::make_unique<ParameterAttachment> (*param, [this] (float v) { value = v; repaint(); }, nullptr);
        attach->sendInitialUpdate();
    }

    int requiredWidth() const { return jmax (50, (int) (toggle ? 1 : items.size()) * 44); }
    static int requiredHeight() { return 14 + 21 + 5 + 12; }

    void paint (Graphics& g) override
    {
        g.setColour (kInk2);
        g.setFont (uiFont (11.f));
        g.drawText (label.toUpperCase(), 0, 0, getWidth(), 13, Justification::centred, false);

        const int n = toggle ? 1 : (int) items.size();
        const int total = n * 44;
        const int x0 = (getWidth() - total) / 2;
        for (int i = 0; i < n; ++i)
        {
            const bool on = toggle ? value > 0.5f : (roundToInt (value) == i);
            const auto lamp = Rectangle<float> ((float) (x0 + i * 44 + 4), 15.f, 36.f, 21.f);
            ColourGradient hg (Colour (0xff3d4147), 0.f, lamp.getY(), Colour (0xff1b1d20), 0.f, lamp.getBottom(), false);
            g.setGradientFill (hg);
            g.fillRoundedRectangle (lamp, 3.f);
            g.setColour (Colours::black);
            g.drawRoundedRectangle (lamp.reduced (0.5f), 3.f, 1.f);

            const auto lens = lamp.reduced (5.f, 4.f);
            if (on)
            {
                DropShadow (led.withAlpha (0.9f), 9, { 0, 0 }).drawForRectangle (g, lens.toNearestInt());
                ColourGradient lg (led.interpolatedWith (Colours::white, 0.65f), 0.f, lens.getY(), led, 0.f, lens.getBottom(), false);
                g.setGradientFill (lg);
            }
            else
                g.setColour (led.interpolatedWith (Colour (0xff151618), 0.78f));
            g.fillRoundedRectangle (lens, 2.f);

            if (! toggle)
            {
                g.setColour (on ? kInk : kInk2);
                g.setFont (uiFont (10.5f));
                g.drawText (items[(size_t) i].toUpperCase(), x0 + i * 44 - 4, 38, 52, 12, Justification::centred, false);
            }
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        const int n = toggle ? 1 : (int) items.size();
        const int x0 = (getWidth() - n * 44) / 2;
        const int i = (e.x - x0) / 44;
        if (e.y < 12 || e.x < x0 || i < 0 || i >= n)
            return;
        attach->setValueAsCompleteGesture (toggle ? (value > 0.5f ? 0.f : 1.f) : (float) i);
    }

private:
    String label;
    std::vector<String> items;
    Colour led;
    bool toggle;
    float value = 0.f;
    std::unique_ptr<ParameterAttachment> attach;
};

//==============================================================================
// Joystick vectoriel : un point libre, ou les 4 points de l'enveloppe vectorielle
// (on saisit le point le plus proche).
class VectorPad : public Component, private Timer
{
public:
    explicit VectorPad (ProphorcelProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        ColourGradient bg (Colour (0xff272c31), b.getCentreX(), b.getHeight() * 0.4f, Colour (0xff121417), b.getCentreX(), b.getHeight() * 1.1f, true);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (b, 6.f);
        g.setColour (Colours::black);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.f, 1.f);

        const auto f = field();
        auto X = [&] (float x) { return f.getX() + x * f.getWidth(); };
        auto Y = [&] (float y) { return f.getY() + y * f.getHeight(); };
        const bool venv = val ("venv") > 0.5f, loop = val ("vloop") > 0.5f;
        const float lx = venv ? proc.engine.getLiveX() : val ("vx");
        const float ly = venv ? proc.engine.getLiveY() : val ("vy");

        const float gains[4] = { (1 - lx) * (1 - ly), lx * (1 - ly), (1 - lx) * ly, lx * ly };
        const float cx[4] = { 0, 1, 0, 1 }, cy[4] = { 0, 0, 1, 1 };
        {
            Graphics::ScopedSaveState ss (g);
            Path clip;
            clip.addRoundedRectangle (b, 6.f);
            g.reduceClipRegion (clip);
            for (int i = 0; i < 4; ++i)
            {
                const float rad = f.getWidth() * (0.12f + 0.55f * gains[i]);
                ColourGradient cg (kOsc[i].withAlpha (0.62f), X (cx[i]), Y (cy[i]), kOsc[i].withAlpha (0.f), X (cx[i]) + rad, Y (cy[i]), true);
                g.setGradientFill (cg);
                g.fillRect (b);
            }
        }

        g.setColour (Colour (0xff3a3e45));
        for (int q = 0; q <= 4; ++q)
        {
            g.drawLine (X (q / 4.f), f.getY(), X (q / 4.f), f.getBottom());
            g.drawLine (f.getX(), Y (q / 4.f), f.getRight(), Y (q / 4.f));
        }
        g.drawRect (f);

        g.setFont (uiFont (15.f));
        const char* names[4] = { "A", "B", "C", "D" };
        for (int i = 0; i < 4; ++i)
        {
            g.setColour (kOsc[i]);
            g.drawText (names[i], Rectangle<float> (X (cx[i]) + (cx[i] > 0.5f ? 2.f : -22.f), Y (cy[i]) + (cy[i] > 0.5f ? 2.f : -22.f), 20.f, 20.f), Justification::centred);
        }

        if (venv)
        {
            Path path;
            for (int k = 0; k < 4; ++k)
            {
                const float px = X (val ("vx" + String (k))), py = Y (val ("vy" + String (k)));
                if (k == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
            }
            if (loop)
                path.closeSubPath();
            const float dashes[2] = { 4.f, 4.f };
            Path dashed;
            PathStrokeType (1.f).createDashedStroke (dashed, path, dashes, 2);
            g.setColour (Colour (0xff9aa1ab));
            g.fillPath (dashed);

            g.setFont (uiFont (12.f));
            for (int k = 0; k < 4; ++k)
            {
                const float px = X (val ("vx" + String (k))), py = Y (val ("vy" + String (k)));
                const bool sel = (k == selected);
                g.setColour (sel ? kAmber : Colour (0xff20262e));
                g.fillEllipse (px - 9.f, py - 9.f, 18.f, 18.f);
                g.setColour (sel ? kAmber : Colour (0xff9aa1ab));
                g.drawEllipse (px - 9.f, py - 9.f, 18.f, 18.f, 1.f);
                g.setColour (sel ? Colour (0xff1a0e06) : Colour (0xffece6d8));
                g.drawText (String (k + 1), Rectangle<float> (px - 9.f, py - 9.f, 18.f, 18.f), Justification::centred);
            }
            g.setColour (Colour (0xffece6d8));
            g.fillEllipse (X (lx) - 5.f, Y (ly) - 5.f, 10.f, 10.f);
        }
        else
        {
            g.setColour (kAmber);
            g.drawEllipse (X (lx) - 13.f, Y (ly) - 13.f, 26.f, 26.f, 2.f);
            g.fillEllipse (X (lx) - 4.f, Y (ly) - 4.f, 8.f, 8.f);
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        const auto f = field();
        const float x = (e.position.x - f.getX()) / f.getWidth(), y = (e.position.y - f.getY()) / f.getHeight();
        if (val ("venv") > 0.5f)
        {
            float bd = 1.0e9f;
            for (int k = 0; k < 4; ++k)
            {
                const float dx = val ("vx" + String (k)) - x, dy = val ("vy" + String (k)) - y, d = dx * dx + dy * dy;
                if (d < bd) { bd = d; selected = k; }
            }
        }
        gx = proc.apvts.getParameter (val ("venv") > 0.5f ? "vx" + String (selected) : String ("vx"));
        gy = proc.apvts.getParameter (val ("venv") > 0.5f ? "vy" + String (selected) : String ("vy"));
        if (gx != nullptr) gx->beginChangeGesture();
        if (gy != nullptr) gy->beginChangeGesture();
        mouseDrag (e);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        const auto f = field();
        const float x = jlimit (0.f, 1.f, (e.position.x - f.getX()) / f.getWidth());
        const float y = jlimit (0.f, 1.f, (e.position.y - f.getY()) / f.getHeight());
        if (gx != nullptr) gx->setValueNotifyingHost (x);
        if (gy != nullptr) gy->setValueNotifyingHost (y);
    }

    void mouseUp (const MouseEvent&) override
    {
        if (gx != nullptr) gx->endChangeGesture();
        if (gy != nullptr) gy->endChangeGesture();
        gx = gy = nullptr;
    }

private:
    void timerCallback() override { repaint(); }
    Rectangle<float> field() const { return getLocalBounds().toFloat().reduced (22.f); }
    float val (const String& id) const { return proc.apvts.getRawParameterValue (id)->load(); }

    ProphorcelProcessor& proc;
    int selected = 0;
    RangedAudioParameter *gx = nullptr, *gy = nullptr;
};

//==============================================================================
class Scope : public Component, private Timer
{
public:
    explicit Scope (ProphorcelProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (Graphics& g) override
    {
        g.setColour (Colour (0xff0b0805));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 3.f);
        proc.engine.copyScope (buf);
        int start = 0;
        for (int i = 1; i < proph::kScopeSize / 2; ++i)
            if (buf[i - 1] < 0.f && buf[i] >= 0.f) { start = i; break; }
        const float W = (float) getWidth(), H = (float) getHeight();
        Path p;
        for (int i = 0; i < getWidth(); ++i)
        {
            const float v = buf[start + (i * (proph::kScopeSize / 2)) / jmax (1, getWidth())];
            const float y = H * 0.5f - jlimit (-1.f, 1.f, v * 1.6f) * (H * 0.45f);
            if (i == 0) p.startNewSubPath (0.f, y); else p.lineTo ((float) i, y);
        }
        g.setColour (kAmber);
        g.strokePath (p, PathStrokeType (1.6f));
        (void) W;
    }

private:
    void timerCallback() override { repaint(); }
    ProphorcelProcessor& proc;
    float buf[proph::kScopeSize] = {};
};

//==============================================================================
// Molette (pitch : revient au centre ; mod : reste en place).
class Wheel : public Component
{
public:
    Wheel (const String& labelText, float minV, float maxV, float startV, bool springBack)
        : label (labelText), lo (minV), hi (maxV), start (startV), spring (springBack), value (startV) {}

    std::function<void (float)> onChange;

    void setValueQuiet (float v)
    {
        if (dragging)
            return;
        value = jlimit (lo, hi, v);
        repaint();
    }

    void paint (Graphics& g) override
    {
        const auto w = Rectangle<float> (0.f, 0.f, (float) getWidth(), 96.f);
        g.setColour (Colour (0xff09090a));
        g.fillRoundedRectangle (w, 7.f);
        g.setColour (Colours::black);
        g.drawRoundedRectangle (w.reduced (1.f), 7.f, 2.f);

        const auto roll = w.reduced (4.f);
        {
            Graphics::ScopedSaveState ss (g);
            Path clip;
            clip.addRoundedRectangle (roll, 4.f);
            g.reduceClipRegion (clip);
            const float p = (value - lo) / (hi - lo);
            const float off = std::fmod (-p * 90.f + 900.f, 9.f);
            for (float y = roll.getY() - 9.f + off; y < roll.getBottom() + 9.f; y += 9.f)
            {
                g.setColour (Colour (0xff34373c));
                g.fillRect (roll.getX(), y, roll.getWidth(), 4.f);
                g.setColour (Colour (0xff15171a));
                g.fillRect (roll.getX(), y + 4.f, roll.getWidth(), 4.f);
                g.setColour (Colour (0xff4a4e55));
                g.fillRect (roll.getX(), y + 8.f, roll.getWidth(), 1.f);
            }
            ColourGradient sh (Colours::black.withAlpha (0.75f), roll.getX(), 0.f, Colours::black.withAlpha (0.7f), roll.getRight(), 0.f, false);
            sh.addColour (0.40, Colours::white.withAlpha (0.16f));
            sh.addColour (0.56, Colours::transparentBlack);
            g.setGradientFill (sh);
            g.fillRect (roll);
        }
        g.setColour (kInk2);
        g.setFont (uiFont (11.f));
        g.drawText (label.toUpperCase(), 0, 98, getWidth(), 13, Justification::centred, false);
    }

    void mouseDown (const MouseEvent& e) override { dragging = true; fromEvent (e); }
    void mouseDrag (const MouseEvent& e) override { fromEvent (e); }
    void mouseUp (const MouseEvent&) override
    {
        dragging = false;
        if (spring)
        {
            value = start;
            if (onChange) onChange (value);
        }
        repaint();
    }

    static int height() { return 98 + 13; }

private:
    void fromEvent (const MouseEvent& e)
    {
        const float t = jlimit (0.f, 1.f, 1.f - e.position.y / 96.f);
        value = lo + t * (hi - lo);
        if (onChange) onChange (value);
        repaint();
    }

    String label;
    float lo, hi, start;
    bool spring, dragging = false;
    float value;
};

} // namespace gui
