#pragma once

// Moteur audio du Prophorcel : aucune dépendance à l'interface, seulement juce_audio_basics.
// 4 oscillateurs à tables d'ondes, mélange vectoriel, filtre 24 dB, enveloppes, LFO,
// écho, chorus stéréo, modes poly / mono / unisson, portamento.

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace proph
{
constexpr int kNumWaves = 13;
constexpr int kTableSize = 2048;
constexpr int kLevels = 10;
constexpr int kMaxVoices = 8;   // notes tenues en polyphonie
constexpr int kMaxSlots = 16;   // emplacements, pour laisser finir les notes relâchées
constexpr int kMaxUni = 5;
constexpr int kChunk = 16;      // pas de calcul des enveloppes, du filtre et des hauteurs
constexpr double kPi = 3.14159265358979323846;
constexpr int kScopeSize = 1024;

// Texte UTF-8 (accents) vers juce::String : String(const char*) ne lit que l'ASCII.
inline juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

// Tous les paramètres sont des float (les entiers et booléens aussi), pour pouvoir les lire d'un coup.
struct Params
{
    float oscWave[4] = { 1, 1, 2, 3 };
    float oscSemi[4] = { 0, 0, 0, 0 };
    float oscFine[4] = { 0, 0, 0, 0 };
    float oscLevel[4] = { 1, 1, 1, 1 };
    float vecX = 0.5f, vecY = 0.5f;
    float cut = 3200.f, res = 3.f, fltEnv = 2.f, keytrack = 0.5f;
    float fenvA = 0.01f, fenvD = 0.6f, fenvS = 0.3f, fenvR = 0.5f;
    float aenvA = 0.01f, aenvD = 0.4f, aenvS = 0.8f, aenvR = 0.5f;
    float lfoShape = 0, lfoRate = 5.f, lfoPitch = 0.f, lfoFilt = 0.f;
    float venvOn = 0, venvLoop = 0;
    float venvT[4] = { 0.6f, 0.6f, 0.6f, 0.6f };
    float venvX[4] = { 0, 1, 1, 0 };
    float venvY[4] = { 0, 0, 1, 1 };
    float dlyTime = 0.28f, dlyFb = 0.35f, dlyMix = 0.2f;
    float chMix = 0.3f, chRate = 0.7f, chDepth = 0.5f;
    float mode = 0, uni = 3, spread = 14.f, glide = 0.f, bend = 2.f, velA = 0.7f, velF = 1.2f;
    float volume = 0.7f;
};

//==============================================================================
// Formes d'onde : somme d'harmoniques, avec une table par octave pour limiter le repliement.
class WaveTables
{
public:
    void build (double sampleRate)
    {
        std::vector<float> sinT ((size_t) kTableSize), cosT ((size_t) kTableSize);
        for (int i = 0; i < kTableSize; ++i)
        {
            sinT[(size_t) i] = (float) std::sin (2.0 * kPi * i / kTableSize);
            cosT[(size_t) i] = (float) std::cos (2.0 * kPi * i / kTableSize);
        }

        tables.assign ((size_t) kNumWaves * kLevels * (kTableSize + 1), 0.f);
        std::vector<double> acc ((size_t) kTableSize);

        for (int w = 0; w < kNumWaves; ++w)
        {
            for (int l = 0; l < kLevels; ++l)
            {
                const double maxFreq = 30.0 * std::pow (2.0, l + 1);
                const int numHarm = std::clamp ((int) std::floor (0.45 * sampleRate / maxFreq), 1, 64);
                std::fill (acc.begin(), acc.end(), 0.0);

                for (int k = 1; k <= numHarm; ++k)
                {
                    double re = 0, im = 0;
                    harmonic (w, k, re, im);
                    if (re == 0.0 && im == 0.0)
                        continue;
                    for (int n = 0; n < kTableSize; ++n)
                    {
                        const int idx = (k * n) & (kTableSize - 1);
                        acc[(size_t) n] += re * cosT[(size_t) idx] + im * sinT[(size_t) idx];
                    }
                }

                double peak = 0;
                for (double a : acc)
                    peak = std::max (peak, std::abs (a));
                if (peak < 1.0e-9)
                    peak = 1.0;

                float* t = tables.data() + (size_t) (w * kLevels + l) * (kTableSize + 1);
                for (int n = 0; n < kTableSize; ++n)
                    t[n] = (float) (acc[(size_t) n] / peak);
                t[kTableSize] = t[0];
            }
        }
    }

    const float* get (int wave, int level) const
    {
        return tables.data() + (size_t) (wave * kLevels + level) * (kTableSize + 1);
    }

    static const char* waveName (int w)
    {
        static const char* names[kNumWaves] = { "Sinus", "Dent de scie", "Carré", "Triangle", "Impulsion 25 %", "Orgue",
                                                "Voix", "Digitale", "Verre", "Cuivre", "Cloche", "Nasale", "Aléatoire" };
        return names[std::clamp (w, 0, kNumWaves - 1)];
    }

private:
    static void harmonic (int w, int k, double& re, double& im)
    {
        const double dk = (double) k;
        switch (w)
        {
            case 0:  im = (k == 1) ? 1.0 : 0.0; break;
            case 1:  im = 1.0 / dk; break;
            case 2:  im = (k % 2) ? 1.0 / dk : 0.0; break;
            case 3:  im = (k % 2) ? ((((k - 1) / 2) % 2) ? -1.0 : 1.0) / (dk * dk) : 0.0; break;
            case 4:  re = std::sin (dk * kPi * 0.25) / dk; break;
            case 5:
            {
                switch (k) { case 1: im = 1.0; break; case 2: im = 0.8; break; case 3: im = 0.5; break;
                             case 4: im = 0.6; break; case 6: im = 0.4; break; case 8: im = 0.35; break; default: break; }
                break;
            }
            case 6:  im = (1.0 / dk) * (std::exp (-std::pow (dk - 3.0, 2.0) / 2.0) + 0.7 * std::exp (-std::pow (dk - 8.0, 2.0) / 4.0)) * 3.0; break;
            case 7:  im = (k % 3) ? 1.0 / dk : 0.0; break;
            case 8:  im = (k == 1 || k == 5 || k == 7 || k == 11 || k == 13 || k == 17) ? 1.0 / std::sqrt (dk) : 0.0; break;
            case 9:  im = (1.0 / std::pow (dk, 1.1)) * (1.0 + 1.6 * std::exp (-std::pow (dk - 5.0, 2.0) / 3.0)); break;
            case 10:
            {
                switch (k) { case 1: im = 1.0; break; case 2: im = 0.6; break; case 3: im = 0.3; break;
                             case 5: im = 0.5; break; case 7: im = 0.2; break; case 9: im = 0.1; break; default: break; }
                break;
            }
            case 11: im = (k >= 2 && k <= 6) ? 1.0 / std::sqrt (dk) : (k == 1 ? 0.3 : 0.0); break;
            case 12: im = std::abs (std::sin (dk * 12.9898)) / std::pow (dk, 0.9); break;
            default: break;
        }
    }

    std::vector<float> tables;
};

//==============================================================================
struct EnvState
{
    int stage = 3; // 0 attaque, 1 déclin, 2 relâche, 3 arrêtée
    float value = 0.f;
};

struct Voice
{
    bool active = false, released = false, mono = false, pending = false; // pending : touche relâchée, tenue par la pédale
    int note = 60;
    double pitchSemi = 60.0, targetSemi = 60.0, glideRate = 0.0;
    float lvAmp = 1.f, velShift = 0.f;
    int copies = 1;
    double phase[4][kMaxUni] = {};
    EnvState aenv, fenv;
    float relA = 0.01f, relF = 0.01f;
    double time = 0.0;
    float prevG[4] = { 0, 0, 0, 0 };
    float s1a = 0, s2a = 0, s1b = 0, s2b = 0;
    std::uint64_t age = 0;
};

//==============================================================================
class Engine
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        tables.build (sampleRate);
        echo.assign ((size_t) (sampleRate * 1.6) + 4, 0.f);
        chBuf.assign ((size_t) (sampleRate * 0.1) + 4, 0.f);
        volume.reset (sampleRate, 0.03);
        volume.setCurrentAndTargetValue (p.volume);
        reset();
    }

    void reset()
    {
        for (auto& v : voices)
            v = Voice();
        stackCount = 0;
        monoIdx = -1;
        monoPending = false;
        sustain = false;
        lastNote = -1;
        lfoPhase = 0.0;
        std::fill (echo.begin(), echo.end(), 0.f);
        std::fill (chBuf.begin(), chBuf.end(), 0.f);
        echoPos = chPos = 0;
        std::fill (std::begin (scope), std::end (scope), 0.f);
        scopeIdx = 0;
        scopeHead.store (0);
        echoTime = p.dlyTime;
        chPhase[0] = 0.f; chPhase[1] = 0.33f; chPhase[2] = 0.66f;
    }

    void setParams (const Params& np)
    {
        const int newMode = (int) std::lround (np.mode);
        if (newMode != lastMode)
        {
            lastMode = newMode;
            p = np;
            allNotesOff (true);
        }
        p = np;
        volume.setTargetValue (p.volume);
    }

    void setPitchBend (float norm) { bendNorm = std::clamp (norm, -1.f, 1.f); }

    // Pédale de sustain (CC 64) : les touches relâchées restent tenues jusqu'au relèvement.
    void setSustain (bool on)
    {
        if (sustain == on)
            return;
        sustain = on;
        if (on)
            return;
        for (auto& v : voices)
            if (v.active && ! v.released && v.pending)
            {
                v.pending = false;
                release (v, false);
            }
        if (monoPending)
        {
            monoPending = false;
            if (monoIdx >= 0)
            {
                release (voices[monoIdx], false);
                monoIdx = -1;
            }
        }
    }

    void noteOn (int note, float vel)
    {
        if (lastMode == 0)
        {
            // une note encore tenue par la pédale est relâchée avant d'être rejouée
            for (auto& v : voices)
                if (v.active && ! v.released && v.pending && v.note == note)
                {
                    v.pending = false;
                    release (v, true);
                }
            if (findVoiceByNote (note) >= 0)
                return;

            int held = 0;
            for (auto& v : voices)
                if (v.active && ! v.released)
                    ++held;
            while (held >= kMaxVoices)
            {
                int oldest = -1;
                for (int i = 0; i < kMaxSlots; ++i)
                    if (voices[i].active && ! voices[i].released && (oldest < 0 || voices[i].age < voices[oldest].age))
                        oldest = i;
                if (oldest < 0)
                    break;
                release (voices[oldest], true);
                --held;
            }
            startVoice (voices[allocSlot()], note, vel, lastNote, false);
        }
        else
        {
            if (stackContains (note))
                return;
            if (stackCount < kStackSize)
                stack[stackCount++] = note;

            if (monoIdx >= 0 && (! voices[monoIdx].active || voices[monoIdx].released))
                monoIdx = -1;

            monoPending = false;
            if (monoIdx >= 0)
                glideTo (voices[monoIdx], note);
            else
            {
                monoIdx = allocSlot();
                startVoice (voices[monoIdx], note, vel, lastNote, true);
            }
        }
        lastNote = note;
    }

    void noteOff (int note)
    {
        if (lastMode == 0)
        {
            const int i = findVoiceByNote (note);
            if (i >= 0)
            {
                if (sustain)
                    voices[i].pending = true;
                else
                    release (voices[i], false);
            }
            return;
        }

        int at = -1;
        for (int i = 0; i < stackCount; ++i)
            if (stack[i] == note)
                at = i;
        if (at < 0)
            return;
        for (int i = at; i < stackCount - 1; ++i)
            stack[i] = stack[i + 1];
        --stackCount;

        if (monoIdx < 0)
            return;
        if (stackCount == 0)
        {
            if (sustain)
                monoPending = true;
            else
            {
                release (voices[monoIdx], false);
                monoIdx = -1;
            }
        }
        else if (voices[monoIdx].note == note)
            glideTo (voices[monoIdx], stack[stackCount - 1]);
    }

    void allNotesOff (bool fast)
    {
        for (auto& v : voices)
            if (v.active && ! v.released)
            {
                v.pending = false;
                release (v, fast);
            }
        stackCount = 0;
        monoIdx = -1;
        monoPending = false;
    }

    // Position du joystick de la note la plus récente, pour l'affichage.
    float getLiveX() const { return liveX.load(); }
    float getLiveY() const { return liveY.load(); }

    int getActiveVoiceCount() const
    {
        int n = 0;
        for (auto& v : voices)
            if (v.active)
                ++n;
        return n;
    }

    // Écrit n échantillons stéréo (remplace le contenu de L et R).
    void process (float* L, float* R, int n)
    {
        int pos = 0;
        while (pos < n)
        {
            const int len = std::min (kChunk, n - pos);
            const float lfoVal = lfoWave ((float) lfoPhase, (int) std::lround (p.lfoShape));
            lfoPhase += (double) p.lfoRate * len / sampleRate;
            lfoPhase -= std::floor (lfoPhase);
            const float bendCents = bendNorm * p.bend * 100.f;

            float mix[kChunk] = {};
            for (auto& v : voices)
                if (v.active)
                    renderChunk (v, mix, len, lfoVal, bendCents);

            for (int i = 0; i < len; ++i)
            {
                const float mono = mix[i] * 0.5f;

                // écho
                echoTime += (p.dlyTime - echoTime) * 0.0005f;
                const float ey = readLerp (echo, echoPos, (double) echoTime * sampleRate);
                echo[(size_t) echoPos] = mono + ey * p.dlyFb;
                if (++echoPos >= (int) echo.size())
                    echoPos = 0;
                const float x = mono + ey * p.dlyMix;

                // chorus / ensemble stéréo
                chBuf[(size_t) chPos] = x;
                const float dry = 1.f - p.chMix * 0.35f, wetG = p.chMix * 0.9f;
                float oL = x * dry, oR = x * dry;
                static constexpr float baseDelay[3] = { 0.016f, 0.022f, 0.029f };
                static constexpr float rateMul[3] = { 1.f, 1.17f, 0.83f };
                static constexpr float panL[3] = { 0.9972f, 0.7071f, 0.0785f };
                static constexpr float panR[3] = { 0.0785f, 0.7071f, 0.9972f };
                for (int t = 0; t < 3; ++t)
                {
                    const float lfo = std::sin (2.f * (float) kPi * chPhase[t]);
                    const double d = ((double) baseDelay[t] + (double) lfo * p.chDepth * 0.004) * sampleRate;
                    const float y = readLerp (chBuf, chPos, std::max (1.0, d));
                    oL += y * panL[t] * wetG;
                    oR += y * panR[t] * wetG;
                    chPhase[t] += p.chRate * rateMul[t] / (float) sampleRate;
                    if (chPhase[t] >= 1.f)
                        chPhase[t] -= 1.f;
                }
                if (++chPos >= (int) chBuf.size())
                    chPos = 0;

                const float vol = volume.getNextValue();
                L[pos + i] = softClip (oL * vol);
                R[pos + i] = softClip (oR * vol);
                scope[scopeIdx] = 0.5f * (L[pos + i] + R[pos + i]);
                scopeIdx = (scopeIdx + 1) & (kScopeSize - 1);
            }
            pos += len;
        }
        scopeHead.store (scopeIdx, std::memory_order_relaxed);
        updateLive();
    }

    // Copie les derniers échantillons (du plus ancien au plus récent) pour l'oscilloscope.
    void copyScope (float* dst) const
    {
        const int h = scopeHead.load (std::memory_order_relaxed);
        for (int i = 0; i < kScopeSize; ++i)
            dst[i] = scope[(h + i) & (kScopeSize - 1)];
    }

private:
    static constexpr int kStackSize = 64;

    static float lfoWave (float ph, int shape)
    {
        switch (shape)
        {
            case 1:  return ph < 0.25f ? 4.f * ph : (ph < 0.75f ? 2.f - 4.f * ph : 4.f * ph - 4.f);
            case 2:  return ph < 0.5f ? 1.f : -1.f;
            case 3:  return ph < 0.5f ? 2.f * ph : 2.f * ph - 2.f;
            default: return std::sin (2.f * (float) kPi * ph);
        }
    }

    static float softClip (float x)
    {
        const float a = std::abs (x);
        if (a < 0.7f)
            return x;
        const float y = 0.7f + 0.3f * std::tanh ((a - 0.7f) / 0.3f);
        return x < 0.f ? -y : y;
    }

    static float readLerp (const std::vector<float>& buf, int writePos, double delaySamples)
    {
        const int N = (int) buf.size();
        double rp = (double) writePos - delaySamples;
        while (rp < 0.0)
            rp += N;
        int i0 = (int) rp;
        const float fr = (float) (rp - i0);
        if (i0 >= N)
            i0 -= N;
        int i1 = i0 + 1;
        if (i1 >= N)
            i1 = 0;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * fr;
    }

    static void mixGains (float x, float y, float* g)
    {
        g[0] = (1.f - x) * (1.f - y);
        g[1] = x * (1.f - y);
        g[2] = (1.f - x) * y;
        g[3] = x * y;
    }

    void vectorPos (double t, float& x, float& y) const
    {
        if (p.venvOn < 0.5f)
        {
            x = p.vecX;
            y = p.vecY;
            return;
        }
        const bool loop = p.venvLoop >= 0.5f;
        const int n = loop ? 4 : 3;
        double cyc = 0;
        for (int i = 0; i < n; ++i)
            cyc += std::max ((double) p.venvT[i], 0.03);
        if (loop)
            t = std::fmod (t, cyc);
        else if (t >= cyc)
        {
            x = p.venvX[3];
            y = p.venvY[3];
            return;
        }
        for (int i = 0; i < n; ++i)
        {
            const double d = std::max ((double) p.venvT[i], 0.03);
            if (t < d)
            {
                const int j = (i + 1) % 4;
                const float f = (float) (t / d);
                x = p.venvX[i] + (p.venvX[j] - p.venvX[i]) * f;
                y = p.venvY[i] + (p.venvY[j] - p.venvY[i]) * f;
                return;
            }
            t -= d;
        }
        x = p.venvX[3];
        y = p.venvY[3];
    }

    int findVoiceByNote (int note) const
    {
        for (int i = 0; i < kMaxSlots; ++i)
            if (voices[i].active && ! voices[i].released && ! voices[i].pending && voices[i].note == note)
                return i;
        return -1;
    }

    bool stackContains (int note) const
    {
        for (int i = 0; i < stackCount; ++i)
            if (stack[i] == note)
                return true;
        return false;
    }

    int allocSlot()
    {
        for (int i = 0; i < kMaxSlots; ++i)
            if (! voices[i].active)
                return i;
        int best = -1;
        for (int i = 0; i < kMaxSlots; ++i)
            if (voices[i].released && (best < 0 || voices[i].age < voices[best].age))
                best = i;
        if (best < 0)
            for (int i = 0; i < kMaxSlots; ++i)
                if (best < 0 || voices[i].age < voices[best].age)
                    best = i;
        if (best == monoIdx)
            monoIdx = -1;
        return best;
    }

    void startVoice (Voice& v, int note, float vel, int fromNote, bool mono)
    {
        const float lv0 = std::clamp (vel, 0.05f, 1.f);
        v = Voice();
        v.active = true;
        v.mono = mono;
        v.note = note;
        v.age = ++ageCounter;
        v.copies = (lastMode == 2) ? std::clamp ((int) std::lround (p.uni), 1, kMaxUni) : 1;
        v.targetSemi = note;
        if (p.glide > 0.f && fromNote >= 0 && fromNote != note)
        {
            v.pitchSemi = fromNote;
            v.glideRate = std::abs ((double) note - (double) fromNote) / (double) p.glide;
        }
        else
            v.pitchSemi = note;
        v.velShift = (lv0 - 1.f) * p.velF * 1200.f;
        v.lvAmp = 1.f - p.velA * (1.f - lv0);
        v.aenv.stage = 0;
        v.fenv.stage = 0;
        float x, y;
        vectorPos (0.0, x, y);
        mixGains (x, y, v.prevG);
    }

    void glideTo (Voice& v, int note)
    {
        v.note = note;
        v.targetSemi = note;
        if (p.glide > 0.f)
            v.glideRate = std::abs ((double) note - v.pitchSemi) / (double) p.glide;
        else
        {
            v.pitchSemi = note;
            v.glideRate = 0.0;
        }
    }

    void release (Voice& v, bool fast)
    {
        if (v.released)
            return;
        v.released = true;
        v.relA = (fast ? 0.03f : p.aenvR) * 0.25f + 0.004f;
        v.relF = (fast ? 0.03f : p.fenvR) * 0.25f + 0.004f;
        v.aenv.stage = 2;
        v.fenv.stage = 2;
    }

    void advance (EnvState& e, int len, float a, float d, float s, float rel) const
    {
        switch (e.stage)
        {
            case 0:
                e.value += (float) (len / (std::max (a, 0.003f) * sampleRate));
                if (e.value >= 1.f)
                {
                    e.value = 1.f;
                    e.stage = 1;
                }
                break;
            case 1:
            {
                const float tau = d * 0.25f + 0.002f;
                e.value = s + (e.value - s) * (float) std::exp (-len / (tau * sampleRate));
                break;
            }
            case 2:
                e.value *= (float) std::exp (-len / (rel * sampleRate));
                if (e.value < 1.0e-4f)
                {
                    e.value = 0.f;
                    e.stage = 3;
                }
                break;
            default: break;
        }
    }

    void renderChunk (Voice& v, float* out, int len, float lfoVal, float bendCents)
    {
        const double sr = sampleRate;
        const double dt = (double) len / sr;

        if (v.pitchSemi != v.targetSemi)
        {
            const double step = v.glideRate * dt;
            if (std::abs (v.targetSemi - v.pitchSemi) <= step)
                v.pitchSemi = v.targetSemi;
            else
                v.pitchSemi += (v.targetSemi > v.pitchSemi) ? step : -step;
        }

        const float aPrev = v.aenv.value, fPrev = v.fenv.value;
        advance (v.aenv, len, p.aenvA, p.aenvD, p.aenvS, v.relA);
        advance (v.fenv, len, p.fenvA, p.fenvD, p.fenvS, v.relF);
        const float aNow = v.aenv.value, fNow = v.fenv.value;
        (void) fPrev;

        // mélange vectoriel : gains interpolés sur le pas de calcul
        float gStart[4], gEnd[4], gStep[4];
        for (int i = 0; i < 4; ++i)
            gStart[i] = v.prevG[i];
        v.time += dt;
        float vx, vy;
        vectorPos (v.time, vx, vy);
        mixGains (vx, vy, gEnd);
        for (int i = 0; i < 4; ++i)
        {
            gStep[i] = (gEnd[i] - gStart[i]) / (float) len;
            v.prevG[i] = gEnd[i];
        }

        // oscillateurs
        float tmp[kChunk] = {};
        const float oscNorm = 0.5f / std::sqrt ((float) v.copies);
        for (int i = 0; i < 4; ++i)
        {
            const int w = std::clamp ((int) std::lround (p.oscWave[i]), 0, kNumWaves - 1);
            const float oscGain = oscNorm * std::clamp (p.oscLevel[i], 0.f, 1.f);
            const double semis = v.pitchSemi + (double) p.oscSemi[i];
            for (int c = 0; c < v.copies; ++c)
            {
                const float off = v.copies > 1 ? ((float) c / (float) (v.copies - 1) - 0.5f) * 2.f * p.spread : 0.f;
                const double cents = (double) p.oscFine[i] + off + (double) p.lfoPitch * lfoVal + bendCents;
                double freq = 440.0 * std::pow (2.0, (semis - 69.0) / 12.0 + cents / 1200.0);
                freq = std::clamp (freq, 1.0, 0.49 * sr);
                const int level = std::clamp ((int) std::floor (std::log2 (freq / 30.0)), 0, kLevels - 1);
                const float* tbl = tables.get (w, level);
                const double inc = freq / sr;
                double ph = v.phase[i][c];
                for (int n = 0; n < len; ++n)
                {
                    const double fi = ph * kTableSize;
                    const int i0 = (int) fi;
                    const float fr = (float) (fi - i0);
                    const float y = tbl[i0] + (tbl[i0 + 1] - tbl[i0]) * fr;
                    tmp[n] += y * (gStart[i] + gStep[i] * (float) n) * oscGain;
                    ph += inc;
                    if (ph >= 1.0)
                        ph -= 1.0;
                }
                v.phase[i][c] = ph;
            }
        }

        // filtre 24 dB : deux étages passe-bas, résonance sur le premier
        const float cents = fNow * p.fltEnv * 1200.f + p.lfoFilt * lfoVal + v.velShift;
        const double base = (double) p.cut * std::pow (2.0, (v.pitchSemi - 60.0) / 12.0 * (double) p.keytrack);
        const double fc = std::clamp (base * std::pow (2.0, (double) cents / 1200.0), 20.0, std::min (16000.0, 0.45 * sr));
        const float g = (float) std::tan (kPi * fc / sr);
        const float k1 = 1.f / (0.7071f * std::pow (10.f, p.res / 20.f));
        const float k2 = 1.f / 0.7071f;
        const float a1a = 1.f / (1.f + g * (g + k1)), a2a = g * a1a, a3a = g * a2a;
        const float a1b = 1.f / (1.f + g * (g + k2)), a2b = g * a1b, a3b = g * a2b;

        for (int n = 0; n < len; ++n)
        {
            float x = tmp[n];
            {
                const float v3 = x - v.s2a;
                const float v1 = a1a * v.s1a + a2a * v3;
                const float v2 = v.s2a + a2a * v.s1a + a3a * v3;
                v.s1a = 2.f * v1 - v.s1a;
                v.s2a = 2.f * v2 - v.s2a;
                x = v2;
            }
            {
                const float v3 = x - v.s2b;
                const float v1 = a1b * v.s1b + a2b * v3;
                const float v2 = v.s2b + a2b * v.s1b + a3b * v3;
                v.s1b = 2.f * v1 - v.s1b;
                v.s2b = 2.f * v2 - v.s2b;
                x = v2;
            }
            const float amp = (aPrev + (aNow - aPrev) * (float) n / (float) len) * v.lvAmp;
            out[n] += x * amp;
        }

        if (v.aenv.stage == 3)
            v.active = false;
    }

    void updateLive()
    {
        const Voice* newest = nullptr;
        for (auto& v : voices)
            if (v.active && (newest == nullptr || v.age > newest->age))
                newest = &v;
        float x, y;
        vectorPos (newest != nullptr ? newest->time : 0.0, x, y);
        liveX.store (x);
        liveY.store (y);
    }

    Params p;
    WaveTables tables;
    double sampleRate = 44100.0;
    Voice voices[kMaxSlots];
    std::uint64_t ageCounter = 0;
    int lastMode = 0;
    int stack[kStackSize] = {};
    int stackCount = 0;
    int monoIdx = -1;
    bool monoPending = false, sustain = false;
    int lastNote = -1;
    float bendNorm = 0.f;
    double lfoPhase = 0.0;
    std::vector<float> echo, chBuf;
    int echoPos = 0, chPos = 0;
    float echoTime = 0.28f;
    float chPhase[3] = { 0.f, 0.33f, 0.66f };
    juce::SmoothedValue<float> volume;
    std::atomic<float> liveX { 0.5f }, liveY { 0.5f };
    float scope[kScopeSize] = {};
    int scopeIdx = 0;
    std::atomic<int> scopeHead { 0 };
};

} // namespace proph
