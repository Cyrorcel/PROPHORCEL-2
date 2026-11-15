#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
using P = std::pair<const char*, float>;

struct Preset
{
    const char* name;
    std::vector<P> values;
};

// Les sons de départ : valeurs écrites par-dessus les réglages par défaut.
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list = {
        { "Cristal vectoriel", { { "w0", 0 }, { "s0", 0 }, { "f0", 0 }, { "w1", 10 }, { "s1", 12 }, { "f1", 3 },
                                 { "w2", 8 }, { "s2", 19 }, { "f2", -4 }, { "w3", 7 }, { "s3", 24 }, { "f3", 0 },
                                 { "cut", 9000 }, { "res", 1 }, { "fltenv", 1 }, { "kt", 0.8f },
                                 { "aa", 0.005f }, { "ad", 1.4f }, { "as", 0.25f }, { "ar", 1.6f },
                                 { "fa", 0.01f }, { "fd", 1 }, { "fs", 0.4f }, { "fr", 1 },
                                 { "venv", 1 }, { "vloop", 1 }, { "vt0", 0.9f }, { "vt1", 0.9f }, { "vt2", 0.9f }, { "vt3", 0.9f },
                                 { "dt", 0.34f }, { "dfb", 0.45f }, { "dmix", 0.3f } } },
        { "Cordes de porcelaine", { { "w0", 1 }, { "f0", -9 }, { "w1", 1 }, { "f1", 8 }, { "w2", 3 }, { "s2", 12 },
                                    { "w3", 1 }, { "s3", -12 }, { "f3", 4 }, { "vx", 0.4f }, { "vy", 0.5f },
                                    { "cut", 2200 }, { "res", 1.5f }, { "fltenv", 1.2f }, { "kt", 0.6f },
                                    { "aa", 0.7f }, { "ad", 0.8f }, { "as", 0.8f }, { "ar", 1.4f },
                                    { "lshape", 0 }, { "lrate", 4.6f }, { "lpitch", 9 },
                                    { "chmix", 0.6f }, { "chrate", 0.6f }, { "chdepth", 0.8f } } },
        { "Cuivres VS", { { "w0", 9 }, { "w1", 1 }, { "f1", 6 }, { "w2", 9 }, { "s2", 12 }, { "f2", -5 }, { "w3", 2 }, { "s3", -12 },
                          { "vx", 0.35f }, { "vy", 0.3f }, { "cut", 900 }, { "res", 2 }, { "fltenv", 3.2f }, { "kt", 0.5f },
                          { "fa", 0.08f }, { "fd", 0.5f }, { "fs", 0.5f }, { "fr", 0.3f },
                          { "aa", 0.04f }, { "ad", 0.3f }, { "as", 0.85f }, { "ar", 0.3f } } },
        { "Pad Pupiter 8", { { "w0", 1 }, { "f0", -12 }, { "w1", 1 }, { "f1", 12 }, { "w2", 2 }, { "s2", -12 }, { "w3", 1 }, { "s3", 7 }, { "f3", -6 },
                             { "cut", 1500 }, { "res", 4 }, { "fltenv", 2 }, { "kt", 0.4f },
                             { "aa", 0.9f }, { "ad", 1 }, { "as", 0.7f }, { "ar", 2 },
                             { "lshape", 1 }, { "lrate", 0.4f }, { "lfilt", 500 },
                             { "venv", 1 }, { "vloop", 1 }, { "vt0", 2.2f }, { "vt1", 2.2f }, { "vt2", 2.2f }, { "vt3", 2.2f },
                             { "vx0", 0.1f }, { "vy0", 0.1f }, { "vx1", 0.9f }, { "vy1", 0.2f }, { "vx2", 0.8f }, { "vy2", 0.9f }, { "vx3", 0.2f }, { "vy3", 0.8f },
                             { "chmix", 0.55f }, { "chrate", 0.5f }, { "chdepth", 0.7f } } },
        { "Basse digitale", { { "w0", 7 }, { "s0", -12 }, { "w1", 2 }, { "s1", -12 }, { "f1", 4 }, { "w2", 1 }, { "s2", -24 }, { "w3", 3 }, { "s3", -12 },
                              { "vx", 0.3f }, { "vy", 0.3f }, { "cut", 500 }, { "res", 6 }, { "fltenv", 3.5f }, { "kt", 0.3f },
                              { "fa", 0.002f }, { "fd", 0.25f }, { "fs", 0.1f }, { "fr", 0.15f },
                              { "aa", 0.002f }, { "ad", 0.3f }, { "as", 0.7f }, { "ar", 0.15f },
                              { "dt", 0.2f }, { "dfb", 0.2f }, { "dmix", 0.06f }, { "mode", 1 }, { "glide", 0.04f } } },
        { "Cloches glacées", { { "w0", 10 }, { "w1", 0 }, { "s1", 12 }, { "w2", 10 }, { "s2", 19 }, { "f2", 5 }, { "w3", 8 }, { "s3", 28 },
                               { "cut", 12000 }, { "res", 0.7f }, { "fltenv", 0 }, { "kt", 1 },
                               { "aa", 0.002f }, { "ad", 2.4f }, { "as", 0 }, { "ar", 2.2f },
                               { "venv", 1 }, { "vt0", 0.5f }, { "vt1", 0.7f }, { "vt2", 0.9f }, { "vt3", 1 },
                               { "vx0", 0 }, { "vy0", 0 }, { "vx1", 1 }, { "vy1", 0 }, { "vx2", 0 }, { "vy2", 1 }, { "vx3", 1 }, { "vy3", 1 },
                               { "dt", 0.42f }, { "dfb", 0.5f }, { "dmix", 0.35f } } },
        { "Voix fantôme", { { "w0", 6 }, { "w1", 11 }, { "f1", -7 }, { "w2", 6 }, { "s2", 12 }, { "f2", 7 }, { "w3", 12 }, { "s3", -12 },
                            { "cut", 2400 }, { "res", 5 }, { "fltenv", 1 }, { "kt", 0.6f },
                            { "aa", 0.35f }, { "ad", 0.7f }, { "as", 0.7f }, { "ar", 1.2f },
                            { "lshape", 0 }, { "lrate", 0.7f }, { "lpitch", 6 }, { "lfilt", 700 },
                            { "venv", 1 }, { "vloop", 1 }, { "vt0", 1.2f }, { "vt1", 0.8f }, { "vt2", 1.2f }, { "vt3", 0.8f } } },
        { "Basse mono glissante", { { "w0", 1 }, { "s0", -12 }, { "w1", 2 }, { "s1", -12 }, { "f1", -6 }, { "w2", 1 }, { "s2", -24 }, { "w3", 7 }, { "s3", -12 },
                                    { "vx", 0.35f }, { "vy", 0.35f }, { "cut", 420 }, { "res", 7 }, { "fltenv", 3.2f }, { "kt", 0.35f },
                                    { "fa", 0.002f }, { "fd", 0.3f }, { "fs", 0.15f }, { "fr", 0.2f },
                                    { "aa", 0.003f }, { "ad", 0.3f }, { "as", 0.8f }, { "ar", 0.2f },
                                    { "mode", 1 }, { "glide", 0.09f }, { "velf", 2 },
                                    { "chmix", 0.12f }, { "chrate", 0.5f }, { "chdepth", 0.4f },
                                    { "dt", 0.2f }, { "dfb", 0.15f }, { "dmix", 0.05f } } },
        { "Lead unisson", { { "w0", 1 }, { "w1", 1 }, { "w2", 2 }, { "w3", 4 }, { "s3", 12 },
                            { "vx", 0.5f }, { "vy", 0.3f }, { "cut", 3800 }, { "res", 3 }, { "fltenv", 1.6f }, { "kt", 0.7f },
                            { "fa", 0.01f }, { "fd", 0.5f }, { "fs", 0.5f }, { "fr", 0.4f },
                            { "aa", 0.01f }, { "ad", 0.3f }, { "as", 0.85f }, { "ar", 0.4f },
                            { "lshape", 0 }, { "lrate", 5.5f }, { "lpitch", 6 },
                            { "mode", 2 }, { "uni", 4 }, { "spread", 16 }, { "glide", 0.06f }, { "velf", 1.5f },
                            { "chmix", 0.3f }, { "chrate", 0.8f }, { "chdepth", 0.5f },
                            { "dt", 0.32f }, { "dfb", 0.3f }, { "dmix", 0.18f } } }
    };
    return list;
}
} // namespace

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout ProphorcelProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    int ver = 1;

    auto flt = [&] (const char* id, const char* name, float lo, float hi, float def, float step = 0.f, const char* unit = "", float midPoint = 0.f)
    {
        NormalisableRange<float> r (lo, hi, step);
        if (midPoint > 0.f)
            r.setSkewForCentre (midPoint);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, ver }, proph::u8 (name), r, def, AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto toggle = [&] (const char* id, const char* name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, ver }, proph::u8 (name), def));
    };
    auto choice = [&] (const char* id, const char* name, StringArray items, int def)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, ver }, proph::u8 (name), items, def));
    };

    StringArray waves;
    for (int i = 0; i < proph::kNumWaves; ++i)
        waves.add (proph::u8 (proph::WaveTables::waveName (i)));

    const char* tag[4] = { "A", "B", "C", "D" };
    const int defWave[4] = { 1, 1, 2, 3 };
    for (int i = 0; i < 4; ++i)
    {
        const String n (i);
        choice (("w" + n).toRawUTF8(), (String ("Onde ") + tag[i]).toRawUTF8(), waves, defWave[i]);
        flt (("s" + n).toRawUTF8(), (String ("Demi-tons ") + tag[i]).toRawUTF8(), -24, 24, 0, 1);
        flt (("f" + n).toRawUTF8(), (String ("Accord fin ") + tag[i]).toRawUTF8(), -50, 50, 0, 0.1f, " ct");
    }
    flt ("vx", "Joystick X", 0, 1, 0.5f, 0.001f);
    flt ("vy", "Joystick Y", 0, 1, 0.5f, 0.001f);

    flt ("cut", "Coupure", 40, 16000, 3200, 0.f, " Hz", 1000.f);
    flt ("res", "Résonance", 0, 20, 3, 0.1f, " dB");
    flt ("fltenv", "Enveloppe filtre", 0, 6, 2, 0.1f, " oct");
    flt ("kt", "Suivi clavier", 0, 1, 0.5f, 0.01f);
    flt ("fa", "Filtre attaque", 0.002f, 4, 0.01f, 0.f, " s", 0.3f);
    flt ("fd", "Filtre déclin", 0.02f, 4, 0.6f, 0.f, " s", 0.5f);
    flt ("fs", "Filtre maintien", 0, 1, 0.3f, 0.01f);
    flt ("fr", "Filtre relâche", 0.02f, 5, 0.5f, 0.f, " s", 0.6f);
    flt ("aa", "Amp attaque", 0.002f, 4, 0.01f, 0.f, " s", 0.3f);
    flt ("ad", "Amp déclin", 0.02f, 4, 0.4f, 0.f, " s", 0.5f);
    flt ("as", "Amp maintien", 0, 1, 0.8f, 0.01f);
    flt ("ar", "Amp relâche", 0.02f, 5, 0.5f, 0.f, " s", 0.6f);

    choice ("lshape", "LFO forme", StringArray { proph::u8 ("Sinus"), proph::u8 ("Triangle"), proph::u8 ("Carré"), proph::u8 ("Dent de scie") }, 0);
    flt ("lrate", "LFO vitesse", 0.1f, 20, 5, 0.f, " Hz", 3.f);
    flt ("lpitch", "LFO hauteur", 0, 100, 0, 1, " ct");
    flt ("lfilt", "LFO filtre", 0, 1200, 0, 10, " ct");

    toggle ("venv", "Env. vectorielle", false);
    toggle ("vloop", "Env. vect. boucle", false);
    const float defX[4] = { 0, 1, 1, 0 }, defY[4] = { 0, 0, 1, 1 };
    for (int i = 0; i < 4; ++i)
    {
        const String n (i), h (i + 1);
        flt (("vt" + n).toRawUTF8(), ("Env. vect. durée " + h).toRawUTF8(), 0.05f, 6, 0.6f, 0.f, " s", 1.f);
        flt (("vx" + n).toRawUTF8(), ("Env. vect. X " + h).toRawUTF8(), 0, 1, defX[i], 0.001f);
        flt (("vy" + n).toRawUTF8(), ("Env. vect. Y " + h).toRawUTF8(), 0, 1, defY[i], 0.001f);
    }

    flt ("dt", "Écho durée", 0.05f, 1, 0.28f, 0.01f, " s");
    flt ("dfb", "Écho répétitions", 0, 0.85f, 0.35f, 0.01f);
    flt ("dmix", "Écho mélange", 0, 1, 0.2f, 0.01f);
    flt ("chmix", "Chorus mélange", 0, 1, 0.3f, 0.01f);
    flt ("chrate", "Chorus vitesse", 0.1f, 3, 0.7f, 0.05f, " Hz");
    flt ("chdepth", "Chorus profondeur", 0, 1, 0.5f, 0.01f);

    choice ("mode", "Mode", StringArray { "Polyphonique", "Mono", "Unisson" }, 0);
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "uni", ver }, "Voix en unisson", 2, 5, 3));
    flt ("spread", "Écart unisson", 0, 40, 14, 1, " ct");
    flt ("glide", "Portamento", 0, 2, 0, 0.f, " s", 0.15f);
    flt ("bend", "Amplitude molette", 0, 12, 2, 1, " demi");
    flt ("vela", "Vélocité volume", 0, 1, 0.7f, 0.01f);
    flt ("velf", "Vélocité filtre", 0, 4, 1.2f, 0.1f, " oct");
    flt ("vol", "Volume", 0, 1, 0.7f, 0.01f);
    return layout;
}

//==============================================================================
ProphorcelProcessor::ProphorcelProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Sortie", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PROPHORCEL", createLayout())
{
    for (auto& k : keyTrans)
        k = -1;
    keyMidi.ensureSize (2048);
    merged.ensureSize (4096);
}

bool ProphorcelProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void ProphorcelProcessor::prepareToPlay (double sampleRate, int)
{
    proph::Params p;
    readParams (p);
    engine.setParams (p);
    engine.prepare (sampleRate);
    engine.setParams (p);
    keyboardState.reset();
}

void ProphorcelProcessor::readParams (proph::Params& p) const
{
    for (int i = 0; i < 4; ++i)
    {
        const String n (i);
        p.oscWave[i] = raw (("w" + n).toRawUTF8())->load();
        p.oscSemi[i] = raw (("s" + n).toRawUTF8())->load();
        p.oscFine[i] = raw (("f" + n).toRawUTF8())->load();
        p.venvT[i] = raw (("vt" + n).toRawUTF8())->load();
        p.venvX[i] = raw (("vx" + n).toRawUTF8())->load();
        p.venvY[i] = raw (("vy" + n).toRawUTF8())->load();
    }
    p.vecX = raw ("vx")->load();
    p.vecY = raw ("vy")->load();
    p.cut = raw ("cut")->load();
    p.res = raw ("res")->load();
    p.fltEnv = raw ("fltenv")->load();
    p.keytrack = raw ("kt")->load();
    p.fenvA = raw ("fa")->load();
    p.fenvD = raw ("fd")->load();
    p.fenvS = raw ("fs")->load();
    p.fenvR = raw ("fr")->load();
    p.aenvA = raw ("aa")->load();
    p.aenvD = raw ("ad")->load();
    p.aenvS = raw ("as")->load();
    p.aenvR = raw ("ar")->load();
    p.lfoShape = raw ("lshape")->load();
    p.lfoRate = raw ("lrate")->load();
    p.lfoPitch = raw ("lpitch")->load();
    p.lfoFilt = raw ("lfilt")->load();
    p.venvOn = raw ("venv")->load();
    p.venvLoop = raw ("vloop")->load();
    p.dlyTime = raw ("dt")->load();
    p.dlyFb = raw ("dfb")->load();
    p.dlyMix = raw ("dmix")->load();
    p.chMix = raw ("chmix")->load();
    p.chRate = raw ("chrate")->load();
    p.chDepth = raw ("chdepth")->load();
    p.mode = raw ("mode")->load();
    p.uni = raw ("uni")->load();
    p.spread = raw ("spread")->load();
    p.glide = raw ("glide")->load();
    p.bend = raw ("bend")->load();
    p.velA = raw ("vela")->load();
    p.velF = raw ("velf")->load();
    p.volume = raw ("vol")->load();
}

void ProphorcelProcessor::handleMidi (const MidiMessage& m)
{
    if (m.isNoteOn())
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.allNotesOff (m.isAllSoundOff());
    else if (m.isPitchWheel())
    {
        const float pb = (float) (m.getPitchWheelValue() - 8192) / 8192.f;
        engine.setPitchBend (pb);
        shownBend.store (pb);
    }
    else if (m.isController() && m.getControllerNumber() == 1 && raw ("venv")->load() < 0.5f)
    {
        if (auto* prm = apvts.getParameter ("vy"))
            prm->setValueNotifyingHost (1.f - (float) m.getControllerValue() / 127.f);
    }
}

void ProphorcelProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    // touches de l'écran : transposées par les boutons Oct, la note relâchée est celle qui a été jouée
    keyMidi.clear();
    merged.clear();
    keyboardState.processNextMidiBuffer (keyMidi, 0, n, true);
    const int shift = octShift.load() * 12;
    for (const auto meta : keyMidi)
    {
        auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            const int orig = m.getNoteNumber(), nn = juce::jlimit (0, 127, orig + shift);
            keyTrans[orig] = nn;
            m = MidiMessage::noteOn (m.getChannel(), nn, m.getVelocity());
        }
        else if (m.isNoteOff())
        {
            const int orig = m.getNoteNumber(), nn = keyTrans[orig] >= 0 ? keyTrans[orig] : juce::jlimit (0, 127, orig + shift);
            keyTrans[orig] = -1;
            m = MidiMessage::noteOff (m.getChannel(), nn, m.getVelocity());
        }
        merged.addEvent (m, meta.samplePosition);
    }
    for (const auto meta : midi)
        merged.addEvent (meta.getMessage(), meta.samplePosition);

    const float gb = guiBend.load();
    if (gb != lastGuiBend)
    {
        lastGuiBend = gb;
        engine.setPitchBend (gb);
        shownBend.store (gb);
    }

    proph::Params p;
    readParams (p);
    engine.setParams (p);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);

    int pos = 0;
    for (const auto meta : merged)
    {
        const int t = std::clamp (meta.samplePosition, 0, n);
        if (t > pos)
        {
            engine.process (L + pos, R + pos, t - pos);
            pos = t;
        }
        handleMidi (meta.getMessage());
    }
    if (pos < n)
        engine.process (L + pos, R + pos, n - pos);
    midi.clear();
}

//==============================================================================
int ProphorcelProcessor::getNumPrograms() { return (int) presets().size(); }

const String ProphorcelProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, getNumPrograms()) ? proph::u8 (presets()[(size_t) index].name) : String();
}

void ProphorcelProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;
    currentProgram = index;

    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (prm))
            if (rp->paramID != "vol" && rp->paramID != "bend" && rp->paramID != "vela")
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (auto& kv : presets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (kv.first))
            rp->setValueNotifyingHost (rp->convertTo0to1 (kv.second));
}

void ProphorcelProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void ProphorcelProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

AudioProcessorEditor* ProphorcelProcessor::createEditor() { return new ProphorcelEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ProphorcelProcessor(); }
