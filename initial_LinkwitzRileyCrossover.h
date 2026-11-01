#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
 * 4th-Order Linkwitz-Riley (LR4, 24 dB/oct) 3-Band Crossover Filter
 * Splits stereo audio into Low, Mid, and High bands with flat summed magnitude
 * and aligned phase response.
 */
class LinkwitzRileyCrossover
{
public:
    LinkwitzRileyCrossover() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;

        // Initialize biquad filters for stereo channels (0 = Left, 1 = Right)
        for (int ch = 0; ch < 2; ++ch)
        {
            lp1_1[ch].reset(); lp1_2[ch].reset();
            hp1_1[ch].reset(); hp1_2[ch].reset();
            lp2_1[ch].reset(); lp2_2[ch].reset();
            hp2_1[ch].reset(); hp2_2[ch].reset();
            ap2_1[ch].reset(); ap2_2[ch].reset();
        }

        updateCoefficients(lowCrossoverFreq, highCrossoverFreq);
    }

    void setCrossoverFrequencies(float lowFreq, float highFreq)
    {
        // Clamp frequencies safely
        lowFreq = juce::jlimit(30.0f, 1500.0f, lowFreq);
        highFreq = juce::jlimit(lowFreq * 1.2f, 18000.0f, highFreq);

        if (std::abs(lowFreq - lowCrossoverFreq) > 0.05f || std::abs(highFreq - highCrossoverFreq) > 0.05f)
        {
            lowCrossoverFreq = lowFreq;
            highCrossoverFreq = highFreq;
            updateCoefficients(lowCrossoverFreq, highCrossoverFreq);
        }
    }

    float getLowCrossover() const { return lowCrossoverFreq; }
    float getHighCrossover() const { return highCrossoverFreq; }

    void process(const float* const* inputChannels,
                 float* const* lowOutput,
                 float* const* midOutput,
                 float* const* highOutput,
                 int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                float in = inputChannels[ch][i];

                // Split 1 at lowCrossoverFreq into L (Low) and H1 (Mid + High)
                float lSample = lp1_2[ch].processSample(lp1_1[ch].processSample(in));
                float h1Sample = hp1_1[ch].processSample(hp1_2[ch].processSample(in));

                // Allpass align the low band so its phase aligns with the mid/high split at highCrossoverFreq
                lowOutput[ch][i] = ap2_2[ch].processSample(ap2_1[ch].processSample(lSample));

                // Split 2 on H1 at highCrossoverFreq into Mid and High
                midOutput[ch][i] = lp2_2[ch].processSample(lp2_1[ch].processSample(h1Sample));
                highOutput[ch][i] = hp2_2[ch].processSample(hp2_1[ch].processSample(h1Sample));
            }
        }
    }

private:
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        void reset() { z1 = 0.0f; z2 = 0.0f; }

        inline float processSample(float in)
        {
            float out = in * b0 + z1;
            z1 = in * b1 - a1 * out + z2;
            z2 = in * b2 - a2 * out;
            return out;
        }

        void setCoefficients(float nb0, float nb1, float nb2, float na1, float na2)
        {
            b0 = nb0; b1 = nb1; b2 = nb2;
            a1 = na1; a2 = na2;
        }
    };

    void updateCoefficients(float fLow, float fHigh)
    {
        if (currentSampleRate <= 0.0) return;

        // 2nd order Butterworth coefficients (Q = 1 / sqrt(2))
        const float q = 0.70710678f;

        // Lowpass and Highpass at fLow
        setButterworthLP(fLow, q, lp1_1);
        setButterworthLP(fLow, q, lp1_2);
        setButterworthHP(fLow, q, hp1_1);
        setButterworthHP(fLow, q, hp1_2);

        // Lowpass and Highpass at fHigh
        setButterworthLP(fHigh, q, lp2_1);
        setButterworthLP(fHigh, q, lp2_2);
        setButterworthHP(fHigh, q, hp2_1);
        setButterworthHP(fHigh, q, hp2_2);

        // Allpass 2nd order at fHigh to phase-align Low band
        setAllpass2(fHigh, q, ap2_1);
        setAllpass2(fHigh, q, ap2_2);
    }

    void setButterworthLP(float freq, float q, Biquad biquads[2])
    {
        double omega = 2.0 * juce::MathConstants<double>::pi * (double)freq / currentSampleRate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * (double)q);

        double a0 = 1.0 + alpha;
        float nb0 = (float)(((1.0 - cs) / 2.0) / a0);
        float nb1 = (float)((1.0 - cs) / a0);
        float nb2 = (float)(((1.0 - cs) / 2.0) / a0);
        float na1 = (float)((-2.0 * cs) / a0);
        float na2 = (float)((1.0 - alpha) / a0);

        for (int ch = 0; ch < 2; ++ch)
            biquads[ch].setCoefficients(nb0, nb1, nb2, na1, na2);
    }

    void setButterworthHP(float freq, float q, Biquad biquads[2])
    {
        double omega = 2.0 * juce::MathConstants<double>::pi * (double)freq / currentSampleRate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * (double)q);

        double a0 = 1.0 + alpha;
        float nb0 = (float)(((1.0 + cs) / 2.0) / a0);
        float nb1 = (float)((-(1.0 + cs)) / a0);
        float nb2 = (float)(((1.0 + cs) / 2.0) / a0);
        float na1 = (float)((-2.0 * cs) / a0);
        float na2 = (float)((1.0 - alpha) / a0);

        for (int ch = 0; ch < 2; ++ch)
            biquads[ch].setCoefficients(nb0, nb1, nb2, na1, na2);
    }

    void setAllpass2(float freq, float q, Biquad biquads[2])
    {
        double omega = 2.0 * juce::MathConstants<double>::pi * (double)freq / currentSampleRate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * (double)q);

        double a0 = 1.0 + alpha;
        float nb0 = (float)((1.0 - alpha) / a0);
        float nb1 = (float)((-2.0 * cs) / a0);
        float nb2 = (float)((1.0 + alpha) / a0);
        float na1 = (float)((-2.0 * cs) / a0);
        float na2 = (float)((1.0 - alpha) / a0);

        for (int ch = 0; ch < 2; ++ch)
            biquads[ch].setCoefficients(nb0, nb1, nb2, na1, na2);
    }

    double currentSampleRate = 44100.0;
    float lowCrossoverFreq = 250.0f;
    float highCrossoverFreq = 3500.0f;

    Biquad lp1_1[2], lp1_2[2];
    Biquad hp1_1[2], hp1_2[2];
    Biquad lp2_1[2], lp2_2[2];
    Biquad hp2_1[2], hp2_2[2];
    Biquad ap2_1[2], ap2_2[2];
};
