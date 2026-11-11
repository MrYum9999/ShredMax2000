#pragma once

#include <algorithm>
#include <array>
#include <cmath>

//==============================================================================
// ShredMaxx2000 compressor engine.
//
// Runs at the oversampled rate. Levels inside the engine are "internal units":
// 1.0 is the fixed internal reference (the threshold), which sits at -18 dBFS
// when the Input knob is at 0 dB. The Headroom control sets the clipping rail
// of the internal circuits, from 4 dB to 28 dB above that reference.
//==============================================================================
namespace sm
{
    constexpr float pi = 3.14159265358979f;
    constexpr float referenceDbfs = -18.0f;

    inline float dbToGain (float db) { return std::exp (db * 0.1151292546f); }
    inline float gainToDb (float g)  { return 8.685889638f * std::log (std::max (g, 1.0e-9f)); }
    inline float coeff (float seconds, float sr) { return std::exp (-1.0f / (std::max (seconds, 1.0e-6f) * sr)); }

    //--------------------------------------------------------------------------
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f, z1 = 0.0f, z2 = 0.0f;

        float process (float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
        void reset() { z1 = z2 = 0.0f; }

        void highPass2 (float fc, float q, float sr)
        {
            const float w = 2.0f * pi * fc / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
            const float a0 = 1.0f + al;
            b0 = 0.5f * (1.0f + c) / a0; b1 = -(1.0f + c) / a0; b2 = b0;
            a1 = -2.0f * c / a0;         a2 = (1.0f - al) / a0;
        }
        void highPass1 (float fc, float sr)
        {
            const float k = std::tan (pi * fc / sr), n = 1.0f / (1.0f + k);
            b0 = n; b1 = -n; b2 = 0.0f; a1 = (k - 1.0f) * n; a2 = 0.0f;
        }
        void peak (float fc, float q, float gainDb, float sr)
        {
            const float A = std::pow (10.0f, gainDb / 40.0f);
            const float w = 2.0f * pi * fc / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
            const float a0 = 1.0f + al / A;
            b0 = (1.0f + al * A) / a0; b1 = -2.0f * c / a0; b2 = (1.0f - al * A) / a0;
            a1 = -2.0f * c / a0;       a2 = (1.0f - al / A) / a0;
        }
    };

    //--------------------------------------------------------------------------
    // The eight ratio settings. Each one reconfigures the knee, the threshold
    // offset and the detector, not just the slope.
    enum Mode { kRatio1 = 0, kRatio2, kRatio3, kRatio4, kRatio6, kRatio10, kRatio20, kKaboom, kNumModes };

    struct ModeSpec
    {
        float slope;       // dB of gain change per dB over threshold (0 = no compression)
        float kneeDb;      // width of the parabolic knee
        float offsetDb;    // threshold offset from the internal reference
        float rmsSeconds;  // averaging window of the RMS part of the detector
        float peakWeight;  // how much the instantaneous peak counts (1 = pure peak detector)
        bool  opto;        // separate opto-cell detector and release
        bool  brickWall;   // instantaneous ceiling after the VCA
    };

    inline const ModeSpec& modeSpec (int m)
    {
        static const ModeSpec specs[kNumModes] = {
            {  0.0f,              0.0f,  0.0f, 0.005f,  0.5f,  false, false },  // 1:1   colour box only
            {  1.0f / 2.0f - 1.0f, 30.0f, -6.0f, 0.006f,  0.45f, false, false },  // 2:1   ultra-soft parabolic, 30 dB
            {  1.0f / 3.0f - 1.0f, 26.0f, -5.0f, 0.005f,  0.5f,  false, false },  // 3:1   ultra-soft parabolic, 26 dB
            {  1.0f / 4.0f - 1.0f, 12.0f, -2.0f, 0.002f,  0.85f, false, false },  // 4:1   medium-soft
            {  1.0f / 6.0f - 1.0f, 10.0f, -1.0f, 0.002f,  0.9f,  false, false },  // 6:1   medium-soft
            {  1.0f / 10.0f - 1.0f, 14.0f, -4.0f, 0.010f, 0.3f,  true,  false },  // 10:1  opto
            {  1.0f / 20.0f - 1.0f,  1.0f,  2.0f, 0.0005f, 1.0f, false, false },  // 20:1  hard-knee peak limiter
            { -1.15f,              2.0f, -3.0f, 0.0005f, 1.0f,  false, true  },  // KABOOM: over-limits, then a brick wall
        };
        return specs[std::clamp (m, 0, kNumModes - 1)];
    }

    // Static curve: gain change in dB (<= 0) for a detector level in dB
    inline float computeGainDb (float levelDb, const ModeSpec& m)
    {
        if (std::abs (m.slope) < 1.0e-6f) return 0.0f;
        const float over = levelDb - m.offsetDb;
        const float w = m.kneeDb;
        if (w <= 0.0f)          return over > 0.0f ? m.slope * over : 0.0f;
        if (over <= -0.5f * w)  return 0.0f;
        if (over >=  0.5f * w)  return m.slope * over;
        const float t = over + 0.5f * w;
        return m.slope * t * t / (2.0f * w);   // parabolic knee
    }

    //--------------------------------------------------------------------------
    // Harmonic circuits. Each has a slope of exactly 1 at zero, so they only add
    // harmonics and never change the level of quiet signals.
    inline float frontEnd (float u)     // Class-A input stage: always present, very gentle
    {
        return std::tanh (u + 0.03f * u * u);
    }

    inline float distDeux (float u)     // tube-like: asymmetric, mostly 2nd-order (even) harmonics
    {
        constexpr float k = 1.6f, b = 0.45f;
        static const float tb = std::tanh (k * b);
        static const float norm = 1.0f / (k * (1.0f - tb * tb));
        return (std::tanh (k * (u + b)) - tb) * norm;
    }

    inline float distTrois (float u)    // tape-like: symmetric, mostly 3rd-order (odd) harmonics
    {
        constexpr float k = 2.0f;
        return std::tanh (k * u) / k;
    }

    // Discrete Class-A output amplifier: clean at normal levels, then rounds off
    // asymmetrically (single-ended stages clip one side earlier). Never above 0 dBFS.
    inline float classAOut (float z)
    {
        constexpr float knee = 0.6f;
        const float a = std::abs (z);
        if (a <= knee) return z + 0.012f * z * z;
        const float rail = z > 0.0f ? 1.0f : 0.93f;
        const float o = knee + (rail - knee) * std::tanh ((a - knee) / (rail - knee));
        const float y = z > 0.0f ? o : -o;
        return y + 0.012f * y * y;
    }

    inline float brickCeiling (float y, float ceil)
    {
        const float knee = 0.8f * ceil, a = std::abs (y);
        if (a <= knee) return y;
        const float o = knee + (ceil - knee) * std::tanh ((a - knee) / (ceil - knee));
        return y > 0.0f ? o : -o;
    }

    //--------------------------------------------------------------------------
    struct Settings
    {
        float attackMs   = 3.0f;      // 0.05 .. 30
        float releaseMs  = 300.0f;    // 50 .. 3500
        float outputDb   = 0.0f;      // makeup gain on the compressed signal
        float headroomDb = 18.0f;     // 4 .. 28
        int   mode       = kRatio4;
        bool  scHpf = false, bandBeater = false, link = true;
        bool  audioHpf = false, dist2 = false, dist3 = false;
    };

    struct Meters
    {
        float maxGrDb     = 0.0f;     // most gain reduction in the block (positive dB)
        float thd         = 0.0f;     // estimated total harmonic distortion (fraction)
        float optoMemory  = 0.0f;     // 0..1, how "charged" the opto cell is
    };

    //==========================================================================
    class Engine
    {
    public:
        void prepare (float oversampledRate)
        {
            sr = oversampledRate;
            for (auto& c : ch)
            {
                c.scHp.highPass2 (100.0f, 0.707f, sr);
                c.bandBeater.peak (6000.0f, 1.0f, 12.0f, sr);
                c.audioHp1.highPass1 (80.0f, sr);
                c.audioHp2.highPass2 (80.0f, 1.0f, sr);   // 1st + 2nd order (Q=1) = 3rd-order Butterworth, 18 dB/oct
                c.dcBlock.highPass1 (4.0f, sr);
                for (auto* f : { &c.thdHpU, &c.thdHpY, &c.thdHpZ, &c.thdHpO }) f->highPass1 (20.0f, sr);
            }
            kSmooth   = 1.0f - coeff (0.02f, sr);
            kCharge   = 1.0f - coeff (2.5f, sr);
            kDischarge= 1.0f - coeff (8.0f, sr);
            reset();
        }

        void reset()
        {
            for (auto& c : ch)
            {
                c.scHp.reset(); c.bandBeater.reset(); c.audioHp1.reset(); c.audioHp2.reset();
                c.dcBlock.reset(); c.thdHpU.reset(); c.thdHpY.reset(); c.thdHpZ.reset(); c.thdHpO.reset();
                c.power = 0.0f; c.env = 0.0f; c.optoFast = 0.0f; c.optoSlow = 0.0f; c.memory = 0.0f;
            }
            primed = false;
        }

        Meters process (float* left, float* right, int n, const Settings& s)
        {
            const ModeSpec& m = modeSpec (s.mode);

            // Targets for the smoothed (click-free) controls
            const float tRail = dbToGain (s.headroomDb);
            const float tOut  = dbToGain (s.outputDb + referenceDbfs);
            const float tF[6] = { s.scHpf ? 1.0f : 0.0f, s.bandBeater ? 1.0f : 0.0f, s.audioHpf ? 1.0f : 0.0f,
                                  s.dist2 ? 1.0f : 0.0f, s.dist3 ? 1.0f : 0.0f, s.link ? 1.0f : 0.0f };
            if (! primed)
            {
                rail = tRail; outGain = tOut;
                for (int i = 0; i < 6; ++i) fade[i] = tF[i];
                primed = true;
            }

            // Ballistics
            const float attackSec  = std::clamp (s.attackMs,  0.05f, 30.0f)   * 0.001f;
            const float releaseSec = std::clamp (s.releaseMs, 50.0f, 3500.0f) * 0.001f;
            const float aAtt  = coeff (attackSec, sr);
            const float aRel  = coeff (releaseSec, sr);
            const float kRms  = 1.0f - coeff (m.rmsSeconds, sr);

            // Opto cell: never quite as fast as the VCA, and a two-stage release whose
            // slow half stretches out (up to 20 s) the longer and harder it has been working.
            const float aOptoAtt  = coeff (attackSec + 0.002f, sr);
            const float aOptoFast = coeff (releaseSec * 0.5f, sr);
            float aOptoSlow[2];
            for (int c = 0; c < 2; ++c)
            {
                const float mem = ch[(size_t) c].memory;
                const float slow = std::min (20.0f, releaseSec * (1.0f + 4.3f * mem) + 1.45f * mem);
                aOptoSlow[c] = coeff (std::max (slow, releaseSec), sr);
            }

            const float brickCeil = dbToGain (m.offsetDb + 1.0f);

            float* io[2] = { left, right };
            float minGr = 0.0f;
            double suu = 0.0, syy = 0.0, suy = 0.0, szz = 0.0, soo = 0.0, szo = 0.0;

            for (int i = 0; i < n; ++i)
            {
                rail    += (tRail - rail) * kSmooth;
                outGain += (tOut - outGain) * kSmooth;
                for (int k = 0; k < 6; ++k) fade[k] += (tF[k] - fade[k]) * kSmooth;

                float x[2], gr[2];

                // ---- Eardrum: how the compressor listens -----------------------
                for (int c = 0; c < 2; ++c)
                {
                    auto& st = ch[(size_t) c];
                    x[c] = io[c][i] / dbToGainRef;

                    float sc = x[c];
                    const float hp = st.scHp.process (sc);
                    sc += fade[0] * (hp - sc);
                    const float bb = st.bandBeater.process (sc);
                    sc += fade[1] * (bb - sc);

                    st.power += (sc * sc - st.power) * kRms;
                    const float level = std::max (std::sqrt (st.power), m.peakWeight * std::abs (sc));
                    const float target = std::max (-60.0f, computeGainDb (gainToDb (level), m));

                    if (m.opto)
                    {
                        st.optoFast = target < st.optoFast ? target + aOptoAtt * (st.optoFast - target)
                                                           : target + aOptoFast * (st.optoFast - target);
                        st.optoSlow = target < st.optoSlow ? target + aOptoAtt * (st.optoSlow - target)
                                                           : target + aOptoSlow[c] * (st.optoSlow - target);
                        st.env = 0.5f * (st.optoFast + st.optoSlow);
                        const float depth = std::min (1.0f, -target / 10.0f);
                        st.memory += (depth - st.memory) * (depth > st.memory ? kCharge : kDischarge);
                    }
                    else
                    {
                        st.env = target < st.env ? target + aAtt * (st.env - target)
                                                 : target + aRel * (st.env - target);
                        st.optoFast = st.optoSlow = st.env;
                        st.memory += (0.0f - st.memory) * kDischarge;
                    }
                }

                // Stereo link sums the two control voltages
                const float linked = 0.5f * (ch[0].env + ch[1].env);
                gr[0] = ch[0].env + fade[5] * (linked - ch[0].env);
                gr[1] = ch[1].env + fade[5] * (linked - ch[1].env);

                // ---- Audio: what you hear --------------------------------------
                for (int c = 0; c < 2; ++c)
                {
                    auto& st = ch[(size_t) c];
                    const float u = x[c] / rail;

                    float y = frontEnd (u);
                    y += fade[3] * (distDeux (y * 0.42f) / 0.42f - y);
                    y += fade[4] * (distTrois (y * 1.2f) / 1.2f - y);

                    // THD estimate: energy not explained by a straight copy of the input
                    const double hu = st.thdHpU.process (u), hy = st.thdHpY.process (y);
                    suu += hu * hu; syy += hy * hy; suy += hu * hy;

                    y *= rail;                                     // back to internal units
                    y *= dbToGain (gr[c]);                         // the VCA
                    if (m.brickWall) y = brickCeiling (y, brickCeil);

                    const float h = st.audioHp2.process (st.audioHp1.process (y));
                    y += fade[2] * (h - y);
                    y = st.dcBlock.process (y);

                    const float z = y * outGain;                   // Output: makeup gain
                    const float o = classAOut (z);
                    const double hz = st.thdHpZ.process (z), ho = st.thdHpO.process (o);
                    szz += hz * hz; soo += ho * ho; szo += hz * ho;
                    io[c][i] = o;

                    minGr = std::min (minGr, gr[c]);
                }
            }

            Meters meters;
            meters.maxGrDb = -minGr;
            double thd2 = 0.0;
            if (suu > 1.0e-7 * n && syy > 0.0) thd2 += std::max (0.0, syy - suy * suy / suu) / (suy * suy / suu + 1.0e-20);
            if (szz > 1.0e-7 * n && soo > 0.0) thd2 += std::max (0.0, soo - szo * szo / szz) / (szo * szo / szz + 1.0e-20);
            meters.thd = (float) std::sqrt (thd2);
            meters.optoMemory = m.opto ? std::max (ch[0].memory, ch[1].memory) : 0.0f;
            return meters;
        }

    private:
        struct Channel
        {
            Biquad scHp, bandBeater, audioHp1, audioHp2, dcBlock, thdHpU, thdHpY, thdHpZ, thdHpO;
            float power = 0.0f, env = 0.0f, optoFast = 0.0f, optoSlow = 0.0f, memory = 0.0f;
        };

        static constexpr float dbToGainRef = 0.1258925412f;   // -18 dBFS
        float sr = 96000.0f;
        float kSmooth = 0.001f, kCharge = 0.0f, kDischarge = 0.0f;
        float rail = 1.0f, outGain = 1.0f;
        float fade[6] {};
        bool primed = false;
        std::array<Channel, 2> ch;
    };
}
