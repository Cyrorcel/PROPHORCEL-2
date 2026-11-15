// Test du moteur seul : rend quelques notes, vérifie qu'il n'y a ni NaN ni débordement,
// et écrit un WAV 16 bits pour l'écoute.
#include "Engine.h"

#include <cstdio>
#include <vector>

static void writeWav (const char* path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path, "wb");
    if (f == nullptr)
        return;
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4, riff = 36 + dataBytes, fmtLen = 16, rate = (uint32_t) sr, byteRate = rate * 4;
    const uint16_t pcm = 1, ch = 2, align = 4, bits = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riff, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmtLen, 4, 1, f); std::fwrite (&pcm, 2, 1, f); std::fwrite (&ch, 2, 1, f);
    std::fwrite (&rate, 4, 1, f); std::fwrite (&byteRate, 4, 1, f); std::fwrite (&align, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i)
    {
        const int16_t l = (int16_t) (std::clamp (L[i], -1.f, 1.f) * 32767.f), r = (int16_t) (std::clamp (R[i], -1.f, 1.f) * 32767.f);
        std::fwrite (&l, 2, 1, f); std::fwrite (&r, 2, 1, f);
    }
    std::fclose (f);
}

struct Result { float peak = 0, rms = 0; bool bad = false; };

static Result render (proph::Engine& e, std::vector<float>& L, std::vector<float>& R, double seconds, int sr)
{
    Result res;
    const int total = (int) (seconds * sr), block = 256;
    double sum = 0;
    int cnt = 0;
    std::vector<float> bl ((size_t) block), br ((size_t) block);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        e.process (bl.data(), br.data(), n);
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (bl[(size_t) i]) || ! std::isfinite (br[(size_t) i]))
                res.bad = true;
            res.peak = std::max ({ res.peak, std::abs (bl[(size_t) i]), std::abs (br[(size_t) i]) });
            sum += (double) bl[(size_t) i] * bl[(size_t) i];
            ++cnt;
            L.push_back (bl[(size_t) i]);
            R.push_back (br[(size_t) i]);
        }
    }
    res.rms = (float) std::sqrt (sum / std::max (1, cnt));
    return res;
}

int main (int argc, char** argv)
{
    const int sr = 48000;
    int failures = 0;
    auto check = [&] (const char* what, bool ok) { std::printf ("%-52s %s\n", what, ok ? "ok" : "ECHEC"); if (! ok) ++failures; };

    proph::Engine e;
    proph::Params p;
    e.prepare (sr);

    // 1. Accord poly sur un son par défaut
    e.setParams (p);
    e.noteOn (60, 0.9f); e.noteOn (64, 0.9f); e.noteOn (67, 0.9f);
    std::vector<float> L, R;
    auto r1 = render (e, L, R, 1.0, sr);
    e.noteOff (60); e.noteOff (64); e.noteOff (67);
    auto r2 = render (e, L, R, 3.0, sr);
    check ("poly : signal présent, pas de NaN", r1.rms > 0.01f && ! r1.bad && ! r2.bad);
    check ("poly : pas de débordement", r1.peak <= 1.0f);
    check ("poly : la queue s'éteint", r2.rms < r1.rms);
    check ("poly : voix libérées à la fin", e.getActiveVoiceCount() == 0);

    // 2. Mono + portamento : enchaînement legato
    p.mode = 1; p.glide = 0.1f;
    e.setParams (p);
    std::vector<float> L2, R2;
    e.noteOn (36, 1.0f);
    render (e, L2, R2, 0.3, sr);
    e.noteOn (43, 1.0f);
    check ("mono : une seule voix après deux notes", e.getActiveVoiceCount() == 1);
    auto r3 = render (e, L2, R2, 0.5, sr);
    e.noteOff (43);
    check ("mono : retour à la note tenue", e.getActiveVoiceCount() == 1);
    e.noteOff (36);
    auto r4 = render (e, L2, R2, 2.5, sr);
    check ("mono : signal valide", r3.rms > 0.01f && ! r3.bad && ! r4.bad);
    check ("mono : voix libérée", e.getActiveVoiceCount() == 0);

    // 3. Unisson, enveloppe vectorielle, pitch bend, vélocité faible
    p.mode = 2; p.uni = 5; p.glide = 0.f; p.venvOn = 1; p.venvLoop = 1; p.res = 12.f; p.lfoPitch = 30.f; p.lfoFilt = 600.f;
    e.setParams (p);
    e.setPitchBend (0.5f);
    std::vector<float> L3, R3;
    e.noteOn (72, 0.2f);
    auto r5 = render (e, L3, R3, 2.5, sr);
    check ("unisson 5 voix + env. vectorielle : valide", r5.rms > 0.0005f && ! r5.bad && r5.peak <= 1.0f);
    e.allNotesOff (false);
    render (e, L3, R3, 2.0, sr);

    // 4. Changement de mode en cours de note, et rafale de notes
    p.mode = 0; p.venvOn = 0; p.res = 20.f; p.cut = 40.f; p.fltEnv = 6.f;
    e.setParams (p);
    std::vector<float> L4, R4;
    for (int i = 0; i < 30; ++i)
    {
        e.noteOn (24 + i * 2, 1.0f);
        render (e, L4, R4, 0.05, sr);
    }
    auto r6 = render (e, L4, R4, 0.5, sr);
    check ("rafale de 30 notes : stable", ! r6.bad && r6.peak <= 1.0f);
    e.allNotesOff (true);
    render (e, L4, R4, 0.5, sr);
    check ("all notes off : tout s'arrête", e.getActiveVoiceCount() == 0);

    // 5. Toutes les formes d'onde, aux deux extrémités du clavier
    bool wavesOk = true;
    for (int w = 0; w < proph::kNumWaves; ++w)
        for (int note : { 12, 60, 108 })
        {
            proph::Params q;
            q.mode = 0;
            for (int i = 0; i < 4; ++i) q.oscWave[i] = (float) w;
            e.setParams (q);
            e.noteOn (note, 1.f);
            std::vector<float> a, b;
            auto rr = render (e, a, b, 0.2, sr);
            wavesOk = wavesOk && ! rr.bad && rr.peak <= 1.0f && rr.rms > 0.0001f;
            e.allNotesOff (true);
            render (e, a, b, 0.2, sr);
        }
    check ("13 formes d'onde × 3 registres : valides", wavesOk);

    if (argc > 1)
    {
        writeWav (argv[1], L, R, sr);
        std::printf ("WAV écrit : %s\n", argv[1]);
    }
    std::printf ("\n%s (%d échec)\n", failures == 0 ? "TOUT EST BON" : "ÉCHECS", failures);
    return failures == 0 ? 0 : 1;
}
