#include "PluginEditor.h"

using namespace juce;
using namespace gui;

namespace
{
void drawWood (Graphics& g, Rectangle<int> r, bool mirror)
{
    const auto clip = g.getClipBounds().getIntersection (r);
    if (clip.isEmpty())
        return;
    for (int x = clip.getX(); x < clip.getRight(); ++x)
    {
        const int k = (x - r.getX()) % 15;
        g.setColour (k < 3 ? Colour (0xffa8693b) : k < 8 ? Colour (0xff96592f) : k < 12 ? Colour (0xffb4764a) : Colour (0xff8b5028));
        g.fillRect (x, clip.getY(), 1, clip.getHeight());
    }
    const float x0 = (float) (mirror ? r.getRight() : r.getX()), x1 = (float) (mirror ? r.getX() : r.getRight());
    ColourGradient sh (Colours::white.withAlpha (0.18f), x0, 0.f, Colours::black.withAlpha (0.28f), x1, 0.f, false);
    sh.addColour (0.22, Colours::transparentBlack);
    sh.addColour (0.70, Colours::transparentBlack);
    g.setGradientFill (sh);
    g.fillRect (r);
}

void drawFace (Graphics& g, Rectangle<int> r)
{
    ColourGradient grad (Colour (0xffe0e3e6), 0.f, (float) r.getY(), Colour (0xffb8bdc3), 0.f, (float) r.getBottom(), false);
    grad.addColour (0.40, Colour (0xffc9cdd2));
    g.setGradientFill (grad);
    g.fillRect (r);
    const auto clip = g.getClipBounds().getIntersection (r);
    for (int x = clip.getX(); x < clip.getRight(); ++x)
    {
        g.setColour ((x & 1) ? Colours::black.withAlpha (0.035f) : Colours::white.withAlpha (0.16f));
        g.fillRect (x, clip.getY(), 1, clip.getHeight());
    }
    ColourGradient l (Colours::black.withAlpha (0.30f), (float) r.getX(), 0.f, Colours::transparentBlack, (float) r.getX() + 8.f, 0.f, false);
    g.setGradientFill (l);
    g.fillRect (r.getX(), r.getY(), 8, r.getHeight());
    ColourGradient rr (Colours::black.withAlpha (0.30f), (float) r.getRight(), 0.f, Colours::transparentBlack, (float) r.getRight() - 8.f, 0.f, false);
    g.setGradientFill (rr);
    g.fillRect (r.getRight() - 8, r.getY(), 8, r.getHeight());
}

void styleLight (ComboBox& b)
{
    b.setColour (ComboBox::backgroundColourId, Colour (0xffefeee9));
    b.setColour (ComboBox::textColourId, kInk);
    b.setColour (ComboBox::outlineColourId, Colour (0xff7d838b));
    b.setColour (ComboBox::arrowColourId, kInk);
}
} // namespace

//==============================================================================
ProphorcelContent::ProphorcelContent (ProphorcelProcessor& p)
    : proc (p), keyboard (p.keyboardState)
{
    setLookAndFeel (&laf);
    auto& apvts = proc.apvts;
    const auto U = [] (const char* s) { return proph::u8 (s); };

    auto knob = [&] (const char* id, const char* label, std::function<String (double)> fmt, Colour mod, int d, int x, int y, int cellW)
    {
        auto& k = make<Knob> (apvts, id, U (label), std::move (fmt), mod, d);
        k.setBounds (x, y, cellW, Knob::heightFor (d));
    };
    auto fader = [&] (const char* id, const char* label, std::function<String (double)> fmt, int x, int y)
    {
        auto& f = make<Fader> (apvts, id, U (label), std::move (fmt));
        f.setBounds (x, y, 42, Fader::height());
    };
    auto leds = [&] (const char* id, const char* label, std::vector<String> names, Colour c, bool toggle, int x, int y, int w)
    {
        auto& l = make<LedGroup> (apvts, id, U (label), std::move (names), c, toggle);
        l.setBounds (x, y, w, LedGroup::requiredHeight());
    };

    //---- en-tête : sons, oscilloscope, volume
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.setColour (ComboBox::backgroundColourId, Colour (0xff14110c));
    presetBox.setColour (ComboBox::textColourId, kAmber);
    presetBox.setColour (ComboBox::outlineColourId, Colour (0xff3a3226));
    presetBox.setColour (ComboBox::arrowColourId, kAmber);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    presetBox.setBounds (752, 32, 210, 28);

    auto& scope = make<Scope> (proc);
    scope.setBounds (980, 28, 360, 36);
    knob ("vol", "Volume", fPct, Colour (0xff6e7178), 64, 1372, 1, 80);

    //---- vecteur
    modules.push_back ({ { 56, 100, 270, 464 }, U ("Vecteur"), kModVec });
    pad = &make<VectorPad> (proc);
    pad->setBounds (69, 134, 244, 244);
    leds ("venv", "Env. vect.", {}, kModVec, true, 101, 388, 90);
    leds ("vloop", "Boucle", {}, kModVec, true, 191, 388, 90);
    const char* tid[4] = { "vt0", "vt1", "vt2", "vt3" };
    const char* tlab[4] = { "1 > 2", "2 > 3", "3 > 4", "4 > 1" };
    for (int i = 0; i < 4; ++i)
        knob (tid[i], tlab[i], fS, kModVec, 44, 67 + i * 62, 448, 62);

    //---- oscillateurs
    modules.push_back ({ { 338, 100, 440, 226 }, U ("Oscillateurs"), kModOsc });
    for (int i = 0; i < 4; ++i)
    {
        const int cx = 350 + i * 105;
        oscCols.push_back ({ cx, 134, 100, 182 });
        auto& box = waveBox[i];
        if (auto* prm = dynamic_cast<AudioParameterChoice*> (apvts.getParameter ("w" + String (i))))
            box.addItemList (prm->choices, 1);
        styleLight (box);
        addAndMakeVisible (box);
        box.setBounds (cx + 2, 146, 96, 24);
        waveAtt[i] = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, "w" + String (i), box);
        knob (("l" + String (i)).toRawUTF8(), "Niveau", fPct, kModOsc, 42, cx, 176, 100);
        knob (("s" + String (i)).toRawUTF8(), "Transp.", fSigned, kModOsc, 36, cx, 250, 50);
        knob (("f" + String (i)).toRawUTF8(), "Fin", fSignedCt, kModOsc, 36, cx + 50, 250, 50);
    }

    //---- filtre
    modules.push_back ({ { 790, 100, 270, 226 }, U ("Filtre passe-bas"), kModFlt });
    knob ("cut", "Coupure", fHz, kModFlt, 76, 796, 138, 96);
    knob ("res", "Réson.", fDb, kModFlt, 48, 896, 150, 52);
    knob ("fltenv", "Env.", fOct, kModFlt, 48, 950, 150, 52);
    knob ("kt", "Suivi", fPct, kModFlt, 48, 1004, 150, 52);

    //---- enveloppes
    modules.push_back ({ { 1072, 100, 180, 226 }, U ("Enveloppe filtre"), kModEnv });
    modules.push_back ({ { 1264, 100, 180, 226 }, U ("Enveloppe ampli"), kModEnv });
    const char* fid[4] = { "fa", "fd", "fs", "fr" };
    const char* aid[4] = { "aa", "ad", "as", "ar" };
    const char* elab[4] = { "Att.", "Décl.", "Maint.", "Rel." };
    for (int i = 0; i < 4; ++i)
    {
        std::function<String (double)> f = (i == 2) ? std::function<String (double)> (fPct) : std::function<String (double)> (fS);
        fader (fid[i], elab[i], f, 1078 + i * 42, 140);
        fader (aid[i], elab[i], f, 1270 + i * 42, 140);
    }

    //---- LFO
    modules.push_back ({ { 338, 338, 250, 226 }, U ("LFO"), kModLfo });
    leds ("lshape", "Forme", { "Sin", "Tri", "Car", "Dent" }, kModLfo, false, 375, 372, 176);
    knob ("lrate", "Vitesse", fHz1, kModLfo, 48, 373, 440, 60);
    knob ("lpitch", "Hauteur", fCt, kModLfo, 48, 433, 440, 60);
    knob ("lfilt", "Filtre", fCt, kModLfo, 48, 493, 440, 60);

    //---- voix
    modules.push_back ({ { 600, 338, 440, 226 }, U ("Voix"), kModVoi });
    leds ("mode", "Mode", { "Poly", "Mono", "Unisson" }, kModVoi, false, 612, 372, 132);
    knob ("uni", "Unisson", [] (double v) { return String (roundToInt (v)) + " voix"; }, kModVoi, 44, 752, 372, 64);
    knob ("spread", "Écart", fCt, kModVoi, 44, 818, 372, 64);
    knob ("glide", "Portamento", [] (double v) { return v < 0.005 ? String ("Off") : fS (v); }, kModVoi, 48, 884, 372, 80);
    knob ("bend", "Molette", [] (double v) { return proph::u8 ("± ") + String (roundToInt (v)) + " dm"; }, kModVoi, 44, 703, 452, 78);
    knob ("vela", "Vél. volume", fPct, kModVoi, 44, 781, 452, 78);
    knob ("velf", "Vél. filtre", fOct, kModVoi, 44, 859, 452, 78);

    //---- chorus, écho
    modules.push_back ({ { 1052, 338, 190, 226 }, U ("Chorus ensemble"), kModFx });
    knob ("chmix", "Mélange", fPct, kModFx, 48, 1060, 380, 58);
    knob ("chrate", "Vitesse", fHzLow, kModFx, 48, 1118, 380, 58);
    knob ("chdepth", "Profond.", fPct, kModFx, 48, 1176, 380, 58);
    modules.push_back ({ { 1254, 338, 190, 226 }, U ("Écho"), kModFx });
    knob ("dt", "Durée", fS, kModFx, 48, 1262, 380, 58);
    knob ("dfb", "Répét.", fPct, kModFx, 48, 1320, 380, 58);
    knob ("dmix", "Mélange", fPct, kModFx, 48, 1378, 380, 58);

    //---- molettes, octaves, clavier
    pitchWheel = &make<Wheel> (U ("Pitch"), -1.f, 1.f, 0.f, true);
    pitchWheel->setBounds (60, 596, 36, Wheel::height());
    pitchWheel->onChange = [this] (float v) { proc.guiBend.store (v); };
    modWheel = &make<Wheel> (U ("Mod"), 0.f, 1.f, 0.5f, false);
    modWheel->setBounds (106, 596, 36, Wheel::height());
    modWheel->onChange = [this] (float v)
    {
        if (proc.apvts.getRawParameterValue ("venv")->load() < 0.5f)
            if (auto* vy = proc.apvts.getParameter ("vy"))
                vy->setValueNotifyingHost (1.f - v);
    };

    for (auto* b : { &octDown, &octUp })
    {
        b->setColour (TextButton::textColourOffId, Colour (0xffece6d8));
        b->setColour (TextButton::textColourOnId, Colour (0xffece6d8));
        addAndMakeVisible (*b);
    }
    octDown.setBounds (58, 716, 64, 24);
    octUp.setBounds (128, 716, 64, 24);
    octDown.onClick = [this] { proc.octShift.store (jmax (-3, proc.octShift.load() - 1)); updateOctLabel(); };
    octUp.onClick = [this] { proc.octShift.store (jmin (3, proc.octShift.load() + 1)); updateOctLabel(); };
    updateOctLabel();

    addAndMakeVisible (keyboard);
    keyboard.setAvailableRange (36, 84);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setKeyWidth (1236.f / 29.f);
    keyboard.setLowestVisibleKey (36);
    keyboard.setBlackNoteLengthProportion (0.62f);
    keyboard.setVelocity (0.9f, true);
    keyboard.setBounds (208, 603, 1236, 150);

    startTimerHz (30);
}

ProphorcelContent::~ProphorcelContent()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ProphorcelContent::updateOctLabel()
{
    const int s = proc.octShift.load();
    octText = "Do " + String (2 + s) + " " + proph::u8 ("→") + " Do " + String (6 + s);
    repaint (56, 742, 140, 16);
}

String ProphorcelContent::hintText() const
{
    return proc.apvts.getRawParameterValue ("venv")->load() > 0.5f
        ? proph::u8 ("Saisis un point numéroté et glisse-le. À chaque note, le mélange parcourt 1 → 4.")
        : proph::u8 ("Glisse le point : plus il approche d'un coin, plus l'oscillateur domine.");
}

void ProphorcelContent::timerCallback()
{
    const bool venv = proc.apvts.getRawParameterValue ("venv")->load() > 0.5f;
    if (! venv)
        modWheel->setValueQuiet (1.f - proc.apvts.getRawParameterValue ("vy")->load());
    pitchWheel->setValueQuiet (proc.shownBend.load());

    const int want = proc.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != want)
        presetBox.setSelectedId (want, dontSendNotification);

    const auto h = hintText();
    if (h != lastHint)
    {
        lastHint = h;
        repaint (60, 524, 256, 40);
    }
}

void ProphorcelContent::paint (Graphics& g)
{
    drawWood (g, { 0, 0, 40, kDesignH }, false);
    drawWood (g, { kDesignW - 40, 0, 40, kDesignH }, true);
    drawFace (g, { 40, 0, kDesignW - 80, kDesignH });

    // logo
    const Colour dots[4] = { kOsc[0], kOsc[1], kOsc[2], kOsc[3] };
    for (int i = 0; i < 4; ++i)
    {
        const float x = 58.f + (float) (i % 2) * 19.f, y = 28.f + (float) (i / 2) * 19.f;
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (x - 1.f, y - 1.f, 17.f, 17.f);
        g.setColour (dots[i]);
        g.fillEllipse (x, y, 15.f, 15.f);
    }
    g.setColour (Colours::white.withAlpha (0.7f));
    g.setFont (uiFont (38.f));
    g.drawText ("PROPHORCEL", 107, 19, 520, 42, Justification::centredLeft, false);
    g.setColour (kInk);
    g.drawText ("PROPHORCEL", 106, 18, 520, 42, Justification::centredLeft, false);
    g.setColour (kInk2);
    g.setFont (uiFont (11.5f));
    g.drawText (proph::u8 ("SYNTHÉTISEUR VECTORIEL · 4 OSCILLATEURS · 8 VOIX"), 106, 62, 600, 14, Justification::centredLeft, false);

    // afficheur
    const auto lcd = Rectangle<float> (740.f, 18.f, 610.f, 56.f);
    g.setColour (Colour (0xff120d07));
    g.fillRoundedRectangle (lcd, 6.f);
    g.setColour (Colour (0xff3a3a3c));
    g.drawRoundedRectangle (lcd.reduced (1.f), 6.f, 2.f);

    // modules
    for (auto& m : modules)
    {
        const auto r = m.r.toFloat();
        ColourGradient body (Colours::white.withAlpha (0.38f), 0.f, r.getY(), Colours::white.withAlpha (0.12f), 0.f, r.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (r, 6.f);
        g.setColour (Colours::black.withAlpha (0.3f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.f);

        Path head;
        head.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), 25.f, 5.f, 5.f, true, true, false, false);
        ColourGradient hg (m.colour.interpolatedWith (Colours::white, 0.3f), 0.f, r.getY(), m.colour, 0.f, r.getY() + 14.f, false);
        g.setGradientFill (hg);
        g.fillPath (head);
        g.setColour (Colours::black.withAlpha (0.4f));
        g.drawLine (r.getX(), r.getY() + 25.f, r.getRight(), r.getY() + 25.f, 1.f);
        g.setColour (Colour (0xff17140f));
        g.setFont (uiFont (13.f));
        g.drawText (m.title.toUpperCase(), m.r.getX() + 12, m.r.getY(), m.r.getWidth() - 24, 25, Justification::centredLeft, false);
    }

    // colonnes d'oscillateurs
    const char* names[4] = { "A", "B", "C", "D" };
    for (size_t i = 0; i < oscCols.size(); ++i)
    {
        const auto r = oscCols[i].toFloat();
        g.setColour (Colours::black.withAlpha (0.07f));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (kOsc[i]);
        g.fillRect (r.getX() + 2.f, r.getY(), r.getWidth() - 4.f, 3.f);
        const float bx = r.getCentreX() - 12.f, by = r.getY() + 8.f;
        g.setColour (Colours::black.withAlpha (0.4f));
        g.fillEllipse (bx - 1.f, by - 1.f, 26.f, 26.f);
        g.setColour (kOsc[i]);
        g.fillEllipse (bx, by, 24.f, 24.f);
        g.setColour (Colour (0xff111111));
        g.setFont (uiFont (14.f));
        g.drawText (names[i], Rectangle<float> (bx, by, 24.f, 24.f), Justification::centred, false);
    }

    // aide du joystick
    g.setColour (kInk2);
    g.setFont (uiFont (12.5f, false));
    g.drawFittedText (hintText(), 66, 526, 250, 34, Justification::centred, 2);

    // clavier : lit sombre, texte d'aide
    g.setColour (Colour (0xff1a1b1e));
    g.fillRoundedRectangle (208.f, 596.f, 1236.f, 160.f, 4.f);
    g.setColour (kReadout);
    g.setFont (monoFont (10.5f));
    g.drawText (octText, 56, 744, 140, 12, Justification::centred, false);
    g.setColour (kInk2);
    g.setFont (uiFont (11.5f, false));
    g.drawFittedText (proph::u8 ("Clavier d'ordinateur : touches A à P. MIDI : vélocité, molette de pitch, molette 1 sur l'axe vertical du joystick. Double-clic sur un bouton : valeur d'origine."),
                      56, 770, 1388, 24, Justification::centredLeft, 1);
}

//==============================================================================
ProphorcelEditor::ProphorcelEditor (ProphorcelProcessor& p)
    : AudioProcessorEditor (&p), content (p)
{
    addAndMakeVisible (content);
    setResizable (true, true);
    setResizeLimits (750, 400, 2250, 1200);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) kDesignW / (double) kDesignH);
    setSize (1200, 640);
}

void ProphorcelEditor::resized()
{
    const float s = (float) getWidth() / (float) kDesignW;
    content.setTransform (AffineTransform::scale (s));
    content.setBounds (0, 0, kDesignW, kDesignH);
}
