#pragma once

// Klangbild Echo – Bandecho
//  Ein Aufnahmekopf schreibt auf eine Bandschleife, drei Wiedergabeköpfe lesen im Abstand 1x, 2x, 3x.
//  Wow & Flutter modulieren die Bandgeschwindigkeit, die Sättigung sitzt vor dem Band (in der Rückkopplung),
//  jede Wiederholung wird durch die Bandfilter dunkler. Zeitänderungen gleiten wie ein träger Motor (Tonhöhe zieht mit).

#include <cmath>
#include <vector>
#include <algorithm>

namespace echo
{
class TapeLine
{
public:
    void prepare (int minSamples)
    {
        int n = 1;
        while (n < minSamples + 8) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); }
    inline void push (float x) noexcept { w = (w + 1) & mask; buf[(size_t) w] = x; }

    // Verzögerung in Samples (>= 2), kubisch interpoliert
    inline float read (float delay) const noexcept
    {
        const float pos = (float) w - delay;
        const float fl = std::floor (pos);
        const float t = pos - fl;
        const int i = (int) fl;
        const float ym = buf[(size_t) ((i - 1) & mask)], y0 = buf[(size_t) (i & mask)];
        const float y1 = buf[(size_t) ((i + 1) & mask)], y2 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (y1 - ym), c2 = ym - 2.5f * y0 + 2.0f * y1 - 0.5f * y2, c3 = 0.5f * (y2 - ym) + 1.5f * (y0 - y1);
        return ((c3 * t + c2) * t + c1) * t + y0;
    }
private:
    std::vector<float> buf;
    int mask = 0, w = 0;
};

struct OnePole
{
    float a = 0.0f, z = 0.0f;
    void setCutoff (double fc, double sr) { a = 1.0f - (float) std::exp (-6.283185307179586 * fc / sr); }
    inline float lp (float x) noexcept { z += a * (x - z); return z; }
    inline float hp (float x) noexcept { z += a * (x - z); return x - z; }
    void reset() { z = 0.0f; }
};

// Welche Köpfe in welcher Stellung laufen (wie am Wahlschalter eines Bandechos)
inline void headMask (int mode, bool out[3]) noexcept
{
    static constexpr bool m[7][3] = { { true, false, false }, { false, true, false }, { false, false, true },
                                      { true, true, false }, { false, true, true }, { true, false, true }, { true, true, true } };
    for (int k = 0; k < 3; ++k) out[k] = m[std::clamp (mode, 0, 6)][k];
}
} // namespace echo
