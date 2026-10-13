// Offline sanity test of SuperParasitesEngine at several host sample rates, modes and qualities.
// It checks that output is finite, bounded and not silent, that freeze keeps sound going after the input stops,
// and reports how fast it runs. It does NOT check that it sounds like the module.
#include "SuperParasitesEngine.h"
#include <chrono>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

static const char* kModeNames[] = { "granular", "stretch", "looping-delay", "spectral", "oliverb", "resonestor", "beat-repeat", "spectral-clouds" };

struct Result { double energy = 0, maxAbs = 0; bool bad = false; double realtimeFactor = 0; };

// Feeds a 110 Hz saw (silence after `inputSeconds`), pressing capture every `captureEvery` seconds (0 = never).
// Returns energy of the last half.
static Result run (double sr, SuperParasitesEngine::Params p, double seconds, double inputSeconds,
                   double freezeAtSeconds = -1.0, double captureEvery = 0.0)
{
    SuperParasitesEngine e; e.prepare (sr);
    const int N = 512;
    std::vector<float> in (N), outL (N), outR (N);
    double phase = 0, nextCapture = captureEvery;
    Result r;
    const int blocks = (int) (sr * seconds / N);
    auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b)
    {
        const double t = (double) b * N / sr;
        if (freezeAtSeconds >= 0 && t >= freezeAtSeconds) p.freeze = true;
        if (captureEvery > 0 && t >= nextCapture) { e.capture(); nextCapture += captureEvery; }
        const bool on = t < inputSeconds;
        for (int i = 0; i < N; ++i)
        {
            phase += 110.0 / sr; if (phase >= 1.0) phase -= 1.0;
            in[i] = on ? 0.4f * (float) (2.0 * phase - 1.0) : 0.0f;
        }
        e.process (in.data(), in.data(), outL.data(), outR.data(), N, p);
        for (int i = 0; i < N; ++i)
        {
            if (! std::isfinite (outL[i]) || ! std::isfinite (outR[i])) r.bad = true;
            r.maxAbs = std::max (r.maxAbs, (double) std::fabs (outL[i]));
            if (b > blocks / 2) r.energy += outL[i] * outL[i] + outR[i] * outR[i];
        }
    }
    const double elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    r.realtimeFactor = seconds / elapsed;
    return r;
}

int main()
{
    int fails = 0;
    const double rates[] = { 44100.0, 48000.0, 96000.0 };

    printf ("pitch knob curve: 0 -> %.2f st, 0.5 -> %.2f st, 1 -> %.2f st\n",
            SuperParasitesEngine::pitchPotToSemitones (0.0f), SuperParasitesEngine::pitchPotToSemitones (0.5f),
            SuperParasitesEngine::pitchPotToSemitones (1.0f));

    for (double sr : rates)
    {
        // 1) every mode x every quality, wet signal only (capture pressed every second so burst/trigger modes sound)
        for (int mode = 0; mode < SuperParasitesEngine::NumModes; ++mode)
            for (int quality = 0; quality < 4; ++quality)
            {
                SuperParasitesEngine::Params p; p.mode = mode; p.quality = quality; p.dryWet = 1.0f; p.density = 0.8f;
                p.texture = 0.4f; p.size = 0.5f; p.position = 0.4f; p.stereoSpread = 0.6f;
                // Spectral clouds mutes random frequency bands by design: POSITION low = most bands muted (silent at 0).
                if (mode == SuperParasitesEngine::SpectralClouds) p.position = 0.95f;
                const Result r = run (sr, p, 6.0, 6.0, -1.0, 1.0);
                const bool ok = ! r.bad && r.energy > 0.5 && r.maxAbs <= 1.5;
                printf ("sr=%6.0f %-15s quality=%d  energy=%10.2f max=%.2f  %5.1fx realtime  %s\n",
                        sr, kModeNames[mode], quality, r.energy, r.maxAbs, r.realtimeFactor, ok ? "OK" : "FAIL");
                fails += ! ok;
            }

        // 2) freeze: stop feeding input, the frozen buffer should keep sounding
        for (int mode : { 0, 2, 3, 4, 7 })
        {
            SuperParasitesEngine::Params p; p.mode = mode; p.quality = 0; p.dryWet = 1.0f; p.density = 0.8f; p.position = 0.5f; p.size = 0.6f;
            const Result r = run (sr, p, 8.0, 2.5, 2.4);
            const bool ok = ! r.bad && r.energy > 0.05 && r.maxAbs <= 1.5;
            printf ("sr=%6.0f %-15s freeze, input stopped  energy=%10.2f max=%.2f  %s\n", sr, kModeNames[mode], r.energy, r.maxAbs, ok ? "OK" : "FAIL");
            fails += ! ok;
        }

        // 3) switching mode and quality while running must not crash or produce non-finite output
        {
            SuperParasitesEngine e; e.prepare (sr);
            SuperParasitesEngine::Params p; p.dryWet = 0.7f;
            const int N = 256; std::vector<float> in (N), oL (N), oR (N);
            double phase = 0; bool bad = false;
            const int blocks = (int) (sr * 16 / N);
            for (int b = 0; b < blocks; ++b)
            {
                if (b % 40 == 0) { p.mode = (b / 40) % SuperParasitesEngine::NumModes; p.quality = (b / 160) % 4; e.capture(); }
                for (int i = 0; i < N; ++i) { phase += 220.0 / sr; if (phase >= 1.0) phase -= 1.0; in[i] = 0.4f * (float) (2.0 * phase - 1.0); }
                e.process (in.data(), in.data(), oL.data(), oR.data(), N, p);
                for (int i = 0; i < N; ++i) if (! std::isfinite (oL[i]) || ! std::isfinite (oR[i]) || std::fabs (oL[i]) > 2.0f) bad = true;
            }
            printf ("sr=%6.0f mode/quality switching while running  %s\n", sr, bad ? "FAIL" : "OK");
            fails += bad;
        }

        // 4) extreme knob positions in every mode must stay finite and bounded
        for (int mode = 0; mode < SuperParasitesEngine::NumModes; ++mode)
            for (int corner = 0; corner < 4; ++corner)
            {
                SuperParasitesEngine::Params p; p.mode = mode;
                const float lo = 0.0f, hi = 1.0f;
                p.position = (corner & 1) ? hi : lo; p.size = (corner & 2) ? hi : lo; p.density = (corner & 1) ? lo : hi;
                p.texture = (corner & 2) ? lo : hi; p.pitchPot = (corner & 1) ? 1.0f : 0.0f; p.dryWet = 1.0f;
                p.stereoSpread = hi; p.feedback = hi; p.reverb = hi; p.gate = (corner == 3);
                const Result r = run (sr, p, 4.0, 4.0, -1.0, 0.7);
                const bool ok = ! r.bad && r.maxAbs <= 1.5;
                if (! ok) printf ("sr=%6.0f %-15s extreme corner %d  max=%.2f  FAIL\n", sr, kModeNames[mode], corner, r.maxAbs);
                fails += ! ok;
            }

        SuperParasitesEngine e; e.prepare (sr);
        printf ("sr=%6.0f processor rate %.0f, reported latency %d samples\n", sr, e.processorRate(), e.latencyInHostSamples());
    }
    printf (fails ? "\n%d FAILURES\n" : "\nall checks passed\n", fails);
    return fails;
}
