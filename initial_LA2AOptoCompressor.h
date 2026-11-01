#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <atomic>

/**
 * Vintage LA-2A Style Optical Tube Compressor Emulation
 * Models the classic T4 electro-optical attenuator with two-stage release,
 * memory effect, soft knee, and 12AX7/12BH7 tube triode saturation.
 */
class LA2AOptoCompressor
{
public:
    LA2AOptoCompressor() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;

        // Attack coefficient (~10ms)
        attackCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * 0.010)));

        // Fast release (~60ms)
        fastReleaseCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * 0.060)));

        // Slow release (~1.2s default base)
        slowReleaseCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * 1.200)));

        // Memory cell integration & leak (~3.5s)
        memoryChargeCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * 0.500)));
        memoryLeakCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * 3.500)));

        // DC blocker filter coefficient (~10Hz)
        dcBlockerR = static_cast<float>(1.0 - (2.0 * juce::MathConstants<double>::pi * 10.0 / sampleRate));

        reset();
    }

    void reset()
    {
        envelope = 0.0f;
        memoryCell = 0.0f;
        gainReductionDb.store(0.0f, std::memory_order_relaxed);

        for (int ch = 0; ch < 2; ++ch)
        {
            dcIn[ch] = 0.0f;
            dcOut[ch] = 0.0f;
        }
    }

    // Parameters
    void setPeakReduction(float pr) { peakReduction = juce::jlimit(0.0f, 100.0f, pr); }
    void setMakeupGainDb(float gainDb) { makeupGainLinear = juce::Decibels::decibelsToGain(gainDb); }
    void setLimitMode(bool isLimit) { limitMode = isLimit; }
    void setTubeWarmth(float warmthPct) { warmthAmount = juce::jlimit(0.0f, 100.0f, warmthPct) / 100.0f; }
    void setBypass(bool bp) { bypassed = bp; }
    void setMuted(bool m) { muted = m; }

    float getGainReductionDb() const
    {
        return gainReductionDb.load(std::memory_order_relaxed);
    }

    void process(float* const* channelData, int numChannels, int numSamples)
    {
        if (muted)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                juce::FloatVectorOperations::clear(channelData[ch], numSamples);
            gainReductionDb.store(0.0f, std::memory_order_relaxed);
            return;
        }

        if (bypassed)
        {
            gainReductionDb.store(0.0f, std::memory_order_relaxed);
            return;
        }

        // Calculate optical T4 drive based on Peak Reduction
        // Peak reduction: 0 -> no compression, 100 -> heavy compression (threshold ~ -45dBFS)
        const float thresholdDb = 6.0f - (peakReduction * 0.52f);
        const float ratio = limitMode ? 12.0f : 3.2f;
        const float kneeWidthDb = limitMode ? 8.0f : 16.0f; // LA-2A has an exceptionally wide soft knee

        float maxGrSample = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Stereo linked detector (peak/RMS blend)
            float maxAbs = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float absVal = std::abs(channelData[ch][i]);
                if (absVal > maxAbs)
                    maxAbs = absVal;
            }

            // Convert to dB
            float inputDb = (maxAbs > 1e-6f) ? juce::Decibels::gainToDecibels(maxAbs) : -120.0f;

            // Target gain reduction curve with soft optical knee
            float targetGrDb = 0.0f;
            float deltaDb = inputDb - thresholdDb;

            if (peakReduction > 0.1f)
            {
                if (deltaDb <= -kneeWidthDb / 2.0f)
                {
                    targetGrDb = 0.0f;
                }
                else if (deltaDb >= kneeWidthDb / 2.0f)
                {
                    // Linear compression above knee
                    targetGrDb = (deltaDb) * (1.0f - (1.0f / ratio));
                }
                else
                {
                    // Quadratic smooth curve within knee
                    float x = deltaDb + (kneeWidthDb / 2.0f);
                    targetGrDb = (x * x / (2.0f * kneeWidthDb)) * (1.0f - (1.0f / ratio));
                }
            }

            targetGrDb = juce::jmax(0.0f, targetGrDb);

            // T4 Opto Attenuator Ballistics (Attack, Dual-Stage Release, Memory Effect)
            if (targetGrDb > envelope)
            {
                // Attack phase (~10ms)
                envelope += attackCoeff * (targetGrDb - envelope);
                // Charge up optical memory during heavy compression
                memoryCell += memoryChargeCoeff * (targetGrDb - memoryCell);
            }
            else
            {
                // Dual stage release:
                // Fast release handles initial ~50% drop, Slow release handles tail.
                // Memory cell lengthens the slow release time when high GR is sustained.
                memoryCell -= memoryLeakCoeff * memoryCell;
                if (memoryCell < 0.0f) memoryCell = 0.0f;

                float memoryFactor = juce::jlimit(0.0f, 1.0f, memoryCell / 18.0f);
                float effectiveSlowCoeff = slowReleaseCoeff * (1.0f - memoryFactor * 0.65f);

                // Blend between fast 60ms release (50%) and slow 1.2s-4s release
                float currentGrRatio = (targetGrDb > 0.01f) ? (envelope / targetGrDb) : 1.0f;
                float blend = (currentGrRatio > 1.5f) ? 0.35f : 0.75f;
                float releaseRate = (1.0f - blend) * fastReleaseCoeff + blend * effectiveSlowCoeff;

                envelope += releaseRate * (targetGrDb - envelope);
            }

            if (envelope > maxGrSample)
                maxGrSample = envelope;

            // Compute linear gain reduction from envelope dB
            float grLinear = juce::Decibels::decibelsToGain(-envelope);
            float totalGainLinear = grLinear * makeupGainLinear;

            // Apply compression and tube saturation to each channel
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = channelData[ch][i] * totalGainLinear;

                // 12AX7 / 12BH7 Tube Triode Saturation
                if (warmthAmount > 0.001f)
                {
                    sample = processTubeStage(sample, ch);
                }

                channelData[ch][i] = sample;
            }
        }

        // Store peak gain reduction for UI meter (negative dB)
        gainReductionDb.store(-maxGrSample, std::memory_order_relaxed);
    }

private:
    float processTubeStage(float in, int ch)
    {
        // Drive signal into tube non-linearity based on warmth
        float drive = 1.0f + (warmthAmount * 2.2f);
        float driven = in * drive;

        // Asymmetric triode soft clipping: produces rich 2nd and 3rd harmonics
        // Positive and negative half-cycles saturate differently
        float out;
        if (driven >= 0.0f)
        {
            // Soft rounded saturation for positive peaks
            out = std::tanh(driven * (1.0f + 0.15f * warmthAmount));
        }
        else
        {
            // Harder compression / curve for negative peaks (asymmetry)
            out = std::tanh(driven * (1.0f - 0.10f * warmthAmount));
        }

        // Normalize gain back
        out = (out / drive);

        // Blend wet/dry saturation
        float saturated = in * (1.0f - warmthAmount) + out * warmthAmount;

        // DC Blocker filter to prevent DC offset accumulation from asymmetric transfer curve
        float dcFiltered = saturated - dcIn[ch] + dcBlockerR * dcOut[ch];
        dcIn[ch] = saturated;
        dcOut[ch] = dcFiltered;

        return dcFiltered;
    }

    double currentSampleRate = 44100.0;
    float peakReduction = 0.0f;
    float makeupGainLinear = 1.0f;
    bool limitMode = false;
    float warmthAmount = 0.25f; // default 25% tube warmth
    bool bypassed = false;
    bool muted = false;

    // T4 ballistics
    float attackCoeff = 0.05f;
    float fastReleaseCoeff = 0.01f;
    float slowReleaseCoeff = 0.001f;
    float memoryChargeCoeff = 0.005f;
    float memoryLeakCoeff = 0.0005f;

    float envelope = 0.0f;
    float memoryCell = 0.0f;
    std::atomic<float> gainReductionDb{ 0.0f };

    // DC blocker states
    float dcBlockerR = 0.999f;
    float dcIn[2] = { 0.0f, 0.0f };
    float dcOut[2] = { 0.0f, 0.0f };
};

