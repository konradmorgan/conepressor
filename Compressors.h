#pragma once
#include <cmath>
#include <algorithm>

namespace cp
{
    inline float dbToLin (float db) { return std::pow (10.0f, db * 0.05f); }
    inline float linToDb (float x)  { return 20.0f * std::log10 (std::max (x, 1.0e-6f)); }
    inline float clamp01 (float x)  { return std::min (1.0f, std::max (0.0f, x)); }

    // Soft-Knee Gain Computer, liefert Gain Reduction in dB (>= 0)
    inline float softKneeGR (float levelDb, float threshDb, float ratio, float kneeDb)
    {
        const float over  = levelDb - threshDb;
        const float slope = 1.0f - 1.0f / ratio;
        if (2.0f * over < -kneeDb) return 0.0f;
        if (2.0f * std::abs (over) <= kneeDb)
        {
            const float t = over + kneeDb * 0.5f;
            return slope * t * t / (2.0f * kneeDb);
        }
        return slope * over;
    }

    // Variable-Mu: Ratio steigt mit dem Pegel ueber der Schwelle
    inline float variableMuGR (float levelDb, float threshDb)
    {
        const float over = levelDb - threshDb;
        if (over <= 0.0f) return 0.0f;
        const float rMax = 8.0f, w = 15.0f;
        return (1.0f - 1.0f / rMax) * over * over / (over + w);
    }

    // Weiche Roehren-Saettigung mit leichtem Bias (gerade Harmonische)
    inline float saturate (float x, float amt)
    {
        if (amt <= 0.0f) return x;
        const float drive = 1.0f + 3.0f * amt;
        const float bias  = 0.08f * amt;
        return (std::tanh (drive * x + bias) - std::tanh (bias)) / drive;
    }

    // ---------------------------------------------------------------- LA-2A (Opto)
    struct OptoComp
    {
        void prepare (double sampleRate) { sr = sampleRate; reset(); }
        void reset() { gr = 0.0f; mem = 0.0f; }

        void setParams (float peakReduction, bool limit, float makeupDb)
        {
            thresh  = -4.0f - peakReduction * 0.4f;
            ratio   = limit ? 12.0f : 3.0f;
            knee    = limit ? 6.0f  : 10.0f;
            atk     = coef (10.0f);
            memCoef = 1.0f - coef (3000.0f);
            makeup  = dbToLin (makeupDb);
        }

        inline void process (float& l, float& r)
        {
            const float lvl    = linToDb (std::max (std::abs (l), std::abs (r)));
            const float target = softKneeGR (lvl, thresh, ratio, knee);

            mem += (target - mem) * memCoef;

            if (target > gr)
                gr = atk * gr + (1.0f - atk) * target;
            else
            {
                const float relMs = 60.0f + (2500.0f - 60.0f) * clamp01 (mem / 10.0f);
                const float rc = coef (relMs);
                gr = rc * gr + (1.0f - rc) * target;
            }

            const float g = dbToLin (-gr) * makeup;
            l *= g; r *= g;
        }

        float gr = 0.0f;   // aktuelle Gain Reduction (dB, positiv)

    private:
        float coef (float ms) const { return std::exp (-1.0f / (float) (sr * (double) ms * 0.001)); }
        double sr = 44100.0;
        float mem = 0.0f, thresh = -10.0f, ratio = 3.0f, knee = 10.0f, atk = 0.0f, memCoef = 0.0f, makeup = 1.0f;
    };

    // ---------------------------------------------------------------- 670 (Variable-Mu)
    struct VarMuComp
    {
        void prepare (double sampleRate) { sr = sampleRate; reset(); }
        void reset() { gr = 0.0f; mem = 0.0f; }

        void setParams (float inputDb, float threshDb, int timeConstant, float outputDb)
        {
            static const float atkMs[6] = { 0.2f, 0.2f, 0.2f, 0.4f, 0.8f, 0.4f };
            static const float relLo[6] = { 300.0f, 800.0f, 2000.0f, 5000.0f, 2000.0f, 300.0f };
            static const float relHi[6] = { 300.0f, 800.0f, 2000.0f, 5000.0f, 10000.0f, 25000.0f };
            tc      = std::min (5, std::max (0, timeConstant));
            lo      = relLo[tc];
            hi      = relHi[tc];
            atk     = coef (atkMs[tc]);
            memCoef = 1.0f - coef (3000.0f);
            thresh  = threshDb;
            inG     = dbToLin (inputDb);
            outG    = dbToLin (outputDb);
        }

        inline void process (float& l, float& r)
        {
            l *= inG; r *= inG;

            const float lvl    = linToDb (std::max (std::abs (l), std::abs (r)));
            const float target = variableMuGR (lvl, thresh);

            mem += (target - mem) * memCoef;

            if (target > gr)
                gr = atk * gr + (1.0f - atk) * target;
            else
            {
                const float relMs = lo + (hi - lo) * clamp01 (mem / 8.0f);
                const float rc = coef (relMs);
                gr = rc * gr + (1.0f - rc) * target;
            }

            const float g = dbToLin (-gr) * outG;
            l *= g; r *= g;
        }

        float gr = 0.0f;

    private:
        float coef (float ms) const { return std::exp (-1.0f / (float) (sr * (double) ms * 0.001)); }
        double sr = 44100.0;
        int tc = 1;
        float mem = 0.0f, thresh = -18.0f, atk = 0.0f, memCoef = 0.0f, lo = 800.0f, hi = 800.0f, inG = 1.0f, outG = 1.0f;
    };
}
