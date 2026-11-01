#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
 * 4th-Order Linkwitz-Riley (LR4, 24 dB/oct) 5-Band Crossover Filter
 * Built with JUCE's Topology-Preserving Transform (TPT) Linkwitz-Riley engine
 * with complete cascaded Allpass phase-alignment.
 * 
 * Guarantees 0.00 dB magnitude ripple (100% perfectly flat sum) and pristine phase coherence
 * across all 5 bands: Sub (20-90Hz), Low (90-500Hz), Mid (500-1200Hz), Mid-High (1.2k-5kHz), Super High (5k-20kHz).
 */
class LinkwitzRileyCrossover
{
public:
    LinkwitzRileyCrossover() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, samplesPerBlock));
        spec.numChannels = 2;

        // 4 Split Filters
        split1.prepare(spec);
        split2.prepare(spec);
        split3.prepare(spec);
        split4.prepare(spec);

        // 6 Cascaded Allpass Filters for pristine phase alignment
        ap1_at_f2.prepare(spec);
        ap1_at_f3.prepare(spec);
        ap1_at_f4.prepare(spec);

        ap2_at_f3.prepare(spec);
        ap2_at_f4.prepare(spec);

        ap3_at_f4.prepare(spec);

        ap1_at_f2.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
        ap1_at_f3.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
        ap1_at_f4.setType(juce::dsp::LinkwitzRileyFilterType::allpass);

        ap2_at_f3.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
        ap2_at_f4.setType(juce::dsp::LinkwitzRileyFilterType::allpass);

        ap3_at_f4.setType(juce::dsp::LinkwitzRileyFilterType::allpass);

        updateFilters();
    }

    void reset()
    {
        split1.reset();
        split2.reset();
        split3.reset();
        split4.reset();

        ap1_at_f2.reset();
        ap1_at_f3.reset();
        ap1_at_f4.reset();

        ap2_at_f3.reset();
        ap2_at_f4.reset();

        ap3_at_f4.reset();
    }

    void setCrossoverFrequencies(float f1, float f2, float f3, float f4)
    {
        float maxAllowed = static_cast<float>(currentSampleRate * 0.45);
        f1 = juce::jlimit(20.0f, juce::jmin(300.0f, maxAllowed), f1);
        f2 = juce::jlimit(f1 * 1.05f, juce::jmin(1500.0f, maxAllowed), f2);
        f3 = juce::jlimit(f2 * 1.05f, juce::jmin(6000.0f, maxAllowed), f3);
        f4 = juce::jlimit(f3 * 1.05f, juce::jmin(18000.0f, maxAllowed), f4);

        if (std::abs(f1 - freq1) > 0.1f || std::abs(f2 - freq2) > 0.1f ||
            std::abs(f3 - freq3) > 0.1f || std::abs(f4 - freq4) > 0.1f)
        {
            freq1 = f1;
            freq2 = f2;
            freq3 = f3;
            freq4 = f4;
            updateFilters();
        }
    }

    float getFreq1() const { return freq1; }
    float getFreq2() const { return freq2; }
    float getFreq3() const { return freq3; }
    float getFreq4() const { return freq4; }

    void process(const float* const* inputChannels,
                 float* const* band1Output, // Sub
                 float* const* band2Output, // Low
                 float* const* band3Output, // Mid
                 float* const* band4Output, // Mid-High
                 float* const* band5Output, // Super High
                 int numSamples)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* in = inputChannels[ch];
            float* out1 = band1Output[ch];
            float* out2 = band2Output[ch];
            float* out3 = band3Output[ch];
            float* out4 = band4Output[ch];
            float* out5 = band5Output[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                float x = in[i];

                // Split 1 at freq1 (90 Hz)
                float l1 = 0.0f, h1 = 0.0f;
                split1.processSample(ch, x, l1, h1);

                // Split 2 at freq2 (500 Hz)
                float l2 = 0.0f, h2 = 0.0f;
                split2.processSample(ch, h1, l2, h2);

                // Split 3 at freq3 (1200 Hz)
                float l3 = 0.0f, h3 = 0.0f;
                split3.processSample(ch, h2, l3, h3);

                // Split 4 at freq4 (5000 Hz)
                float l4 = 0.0f, h4 = 0.0f;
                split4.processSample(ch, h3, l4, h4);

                // Phase alignment for Band 1 (L1 passed through AP f2 -> f3 -> f4)
                float b1 = ap1_at_f2.processSample(ch, l1);
                b1 = ap1_at_f3.processSample(ch, b1);
                b1 = ap1_at_f4.processSample(ch, b1);

                // Phase alignment for Band 2 (L2 passed through AP f3 -> f4)
                float b2 = ap2_at_f3.processSample(ch, l2);
                b2 = ap2_at_f4.processSample(ch, b2);

                // Phase alignment for Band 3 (L3 passed through AP f4)
                float b3 = ap3_at_f4.processSample(ch, l3);

                // Band 4 is L4 (already matches phase at f4)
                float b4 = l4;

                // Band 5 is H4 (matches phase at f4)
                float b5 = h4;

                out1[i] = b1;
                out2[i] = b2;
                out3[i] = b3;
                out4[i] = b4;
                out5[i] = b5;
            }
        }
    }

private:
    void updateFilters()
    {
        if (currentSampleRate <= 0.0) return;

        float nyquist = static_cast<float>(currentSampleRate * 0.49);
        float s1 = juce::jlimit(20.0f, nyquist, freq1);
        float s2 = juce::jlimit(s1 * 1.02f, nyquist, freq2);
        float s3 = juce::jlimit(s2 * 1.02f, nyquist, freq3);
        float s4 = juce::jlimit(s3 * 1.02f, nyquist, freq4);

        split1.setCutoffFrequency(s1);
        split2.setCutoffFrequency(s2);
        split3.setCutoffFrequency(s3);
        split4.setCutoffFrequency(s4);

        ap1_at_f2.setCutoffFrequency(s2);
        ap1_at_f3.setCutoffFrequency(s3);
        ap1_at_f4.setCutoffFrequency(s4);

        ap2_at_f3.setCutoffFrequency(s3);
        ap2_at_f4.setCutoffFrequency(s4);

        ap3_at_f4.setCutoffFrequency(s4);
    }

    double currentSampleRate = 44100.0;

    // Default Crossover Points: 90 Hz, 500 Hz, 1200 Hz, 5000 Hz
    float freq1 = 90.0f;
    float freq2 = 500.0f;
    float freq3 = 1200.0f;
    float freq4 = 5000.0f;

    juce::dsp::LinkwitzRileyFilter<float> split1;
    juce::dsp::LinkwitzRileyFilter<float> split2;
    juce::dsp::LinkwitzRileyFilter<float> split3;
    juce::dsp::LinkwitzRileyFilter<float> split4;

    juce::dsp::LinkwitzRileyFilter<float> ap1_at_f2;
    juce::dsp::LinkwitzRileyFilter<float> ap1_at_f3;
    juce::dsp::LinkwitzRileyFilter<float> ap1_at_f4;

    juce::dsp::LinkwitzRileyFilter<float> ap2_at_f3;
    juce::dsp::LinkwitzRileyFilter<float> ap2_at_f4;

    juce::dsp::LinkwitzRileyFilter<float> ap3_at_f4;
};
