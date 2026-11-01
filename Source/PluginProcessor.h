#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "LinkwitzRileyCrossover.h"
#include "LA2AOptoCompressor.h"
#include "LiveSyncBridge.h"
#include "PresetManager.h"

class MultibandCompressor5BAudioProcessor : public juce::AudioProcessor,
                                           public juce::AudioProcessorValueTreeState::Listener
{
public:
    MultibandCompressor5BAudioProcessor();
    ~MultibandCompressor5BAudioProcessor() override;

    void parameterChanged(const juce::String& paramID, float newValue) override;
    void resetToDefault();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    bool supportsDoublePrecisionProcessing() const override { return true; }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "5 Multiband Compressor"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    PresetManager& getPresetManager() { return presetManager; }

    // Metering and visual data getters for UI
    float getSubGainReduction() const { return subCompressor.getGainReductionDb(); }
    float getLowGainReduction() const { return lowCompressor.getGainReductionDb(); }
    float getMidGainReduction() const { return midCompressor.getGainReductionDb(); }
    float getMidHighGainReduction() const { return midHighCompressor.getGainReductionDb(); }
    float getHighGainReduction() const { return highCompressor.getGainReductionDb(); }

    float getInputLevel() const { return inputLevel.load(std::memory_order_relaxed); }
    float getOutputLevel() const { return outputLevel.load(std::memory_order_relaxed); }

    float getFreq1() const { return crossover[0].getFreq1(); }
    float getFreq2() const { return crossover[0].getFreq2(); }
    float getFreq3() const { return crossover[0].getFreq3(); }
    float getFreq4() const { return crossover[0].getFreq4(); }

    void setOversamplingIndex(int index);
    int getCurrentOversamplingIndex() const;
    int getCurrentLatencySamples() const { return currentLatencyReport.load(std::memory_order_relaxed); }

    void publishLiveParameters();

private:
    void pollLiveParameters();
    LiveSyncBridge liveSync;
    std::atomic<bool> isPollingLive{ false };

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void applyOversamplingRate(int factorIndex);

    template <typename SampleType>
    void processAudio(juce::AudioBuffer<SampleType>& buffer);

    // 5 Cascaded LR4 crossovers pre-prepared for each rate: 1x, 2x, 4x, 8x, 16x
    LinkwitzRileyCrossover crossover[5];
    LA2AOptoCompressor subCompressor;
    LA2AOptoCompressor lowCompressor;
    LA2AOptoCompressor midCompressor;
    LA2AOptoCompressor midHighCompressor;
    LA2AOptoCompressor highCompressor;

    // Multi-stage oversamplers: 0 = 1x (none), 1 = 2x, 2 = 4x, 3 = 8x, 4 = 16x
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[5];
    std::atomic<int> currentOversamplingIndex{ 0 };
    std::atomic<int> currentLatencyReport{ 0 };

    double baseSampleRate = 44100.0;
    static constexpr int CHUNK_SIZE = 1024;
    static constexpr int MAX_PROC_SAMPLES = CHUNK_SIZE * 16; // 16384 samples for 16x

    // Delay line for phase-accurate dry/wet mixing across oversampling latencies
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> dryDelay{ 512 };

    // Pre-allocated static buffers (zero real-time allocation for Equalizer APO stability)
    juce::AudioBuffer<float> chunkInputBuffer;
    juce::AudioBuffer<float> chunkDryBuffer;
    juce::AudioBuffer<float> subBuffer;
    juce::AudioBuffer<float> lowBuffer;
    juce::AudioBuffer<float> midBuffer;
    juce::AudioBuffer<float> midHighBuffer;
    juce::AudioBuffer<float> highBuffer;

    // Smoothed crossover frequencies
    juce::SmoothedValue<float> smoothedF1;
    juce::SmoothedValue<float> smoothedF2;
    juce::SmoothedValue<float> smoothedF3;
    juce::SmoothedValue<float> smoothedF4;

    // Master meters
    std::atomic<float> inputLevel{ 0.0f };
    std::atomic<float> outputLevel{ 0.0f };

    PresetManager presetManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultibandCompressor5BAudioProcessor)
};
