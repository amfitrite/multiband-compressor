#include "PluginProcessor.h"
#include "PluginEditor.h"

MultibandCompressor5BAudioProcessor::MultibandCompressor5BAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      presetManager(apvts)
{
    liveSync.init();

    for (auto* param : getParameters())
    {
        if (auto* pWithId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            apvts.addParameterListener(pWithId->paramID, this);
    }

    if (auto* osParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("oversampling")))
        currentOversamplingIndex.store(osParam->getIndex(), std::memory_order_relaxed);

    cached.oversampling = apvts.getRawParameterValue("oversampling");
    cached.ui_mode = apvts.getRawParameterValue("ui_mode");
    cached.input_gain = apvts.getRawParameterValue("input_gain");
    cached.output_gain = apvts.getRawParameterValue("output_gain");
    cached.mix = apvts.getRawParameterValue("mix");

    cached.sub_low_freq = apvts.getRawParameterValue("sub_low_freq");
    cached.low_mid_freq = apvts.getRawParameterValue("low_mid_freq");
    cached.mid_high_freq = apvts.getRawParameterValue("mid_high_freq");
    cached.high_super_freq = apvts.getRawParameterValue("high_super_freq");

    cached.sub_solo = apvts.getRawParameterValue("sub_solo");
    cached.low_solo = apvts.getRawParameterValue("low_solo");
    cached.mid_solo = apvts.getRawParameterValue("mid_solo");
    cached.mid_high_solo = apvts.getRawParameterValue("mid_high_solo");
    cached.high_solo = apvts.getRawParameterValue("high_solo");

    cached.sub_mute = apvts.getRawParameterValue("sub_mute");
    cached.low_mute = apvts.getRawParameterValue("low_mute");
    cached.mid_mute = apvts.getRawParameterValue("mid_mute");
    cached.mid_high_mute = apvts.getRawParameterValue("mid_high_mute");
    cached.high_mute = apvts.getRawParameterValue("high_mute");

    auto cacheBand = [this](CachedParams::BandParams& bp, const juce::String& prefix) {
        bp.bypass = apvts.getRawParameterValue(prefix + "_bypass");
        bp.mute = apvts.getRawParameterValue(prefix + "_mute");
        bp.gain = apvts.getRawParameterValue(prefix + "_gain");
        bp.warmth = apvts.getRawParameterValue(prefix + "_warmth");

        bp.peak_reduction = apvts.getRawParameterValue(prefix + "_peak_reduction");
        bp.mode = apvts.getRawParameterValue(prefix + "_mode");

        bp.threshold = apvts.getRawParameterValue(prefix + "_threshold");
        bp.ratio = apvts.getRawParameterValue(prefix + "_ratio");
        bp.attack = apvts.getRawParameterValue(prefix + "_attack");
        bp.release = apvts.getRawParameterValue(prefix + "_release");
        bp.knee = apvts.getRawParameterValue(prefix + "_knee");
        bp.band_mix = apvts.getRawParameterValue(prefix + "_band_mix");
        bp.engine = apvts.getRawParameterValue(prefix + "_engine");
    };

    cacheBand(cached.sub, "sub");
    cacheBand(cached.low, "low");
    cacheBand(cached.mid, "mid");
    cacheBand(cached.mid_high, "mid_high");
    cacheBand(cached.high, "high");
}

MultibandCompressor5BAudioProcessor::~MultibandCompressor5BAudioProcessor()
{
    for (auto* param : getParameters())
    {
        if (auto* pWithId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            apvts.removeParameterListener(pWithId->paramID, this);
    }
    liveSync.close();
}

void MultibandCompressor5BAudioProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    if (paramID == "oversampling")
    {
        int idx = juce::jlimit(0, 4, juce::roundToInt(newValue));
        currentOversamplingIndex.store(idx, std::memory_order_relaxed);
        applyOversamplingRate(idx);
    }

    publishLiveParameters();
}

void MultibandCompressor5BAudioProcessor::resetToDefault()
{
    for (auto* param : getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            p->setValueNotifyingHost(p->getDefaultValue());
    }

    setOversamplingIndex(0);
}

juce::AudioProcessorValueTreeState::ParameterLayout MultibandCompressor5BAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global UI Mode & Oversampling & GPU Acceleration
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "ui_mode", 1 }, "UI Mode", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "gpu_accel", 1 }, "GPU Acceleration", false));

    juce::StringArray oversamplingChoices{ "1x (Off)", "2x", "4x", "8x", "16x" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "oversampling", 1 }, "Oversampling", oversamplingChoices, 0));

    // 4 Crossover frequencies (Seamless contiguous LR4 points)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "sub_low_freq", 1 }, "Sub-Low Crossover",
        juce::NormalisableRange<float>(30.0f, 200.0f, 1.0f, 0.4f), 90.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "low_mid_freq", 1 }, "Low-Mid Crossover",
        juce::NormalisableRange<float>(150.0f, 900.0f, 1.0f, 0.4f), 500.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mid_high_freq", 1 }, "Mid-High Crossover",
        juce::NormalisableRange<float>(700.0f, 3000.0f, 1.0f, 0.4f), 1200.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "high_super_freq", 1 }, "High-Super Crossover",
        juce::NormalisableRange<float>(2500.0f, 16000.0f, 1.0f, 0.4f), 5000.0f));

    // Helper lambda to register band parameters
    auto addBandParams = [&](const juce::String& prefix, const juce::String& bandName,
                             float defPr, float defAtt, float defRel, bool defDual,
                             float defWarmth, float defRatio, float defKnee, float defThresh)
    {
        // Normal Mode params
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_peak_reduction", 1 }, bandName + " Peak Reduction",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), defPr));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_gain", 1 }, bandName + " Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ prefix + "_mode", 1 }, bandName + " Limit Mode", false));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_warmth", 1 }, bandName + " Tube Warmth",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), defWarmth));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ prefix + "_bypass", 1 }, bandName + " Bypass", false));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ prefix + "_solo", 1 }, bandName + " Solo", false));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ prefix + "_mute", 1 }, bandName + " Mute", false));

        // Advance Mode params
        juce::StringArray engineChoices{ "Single", "Dual" };
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ prefix + "_engine", 1 }, bandName + " Engine", engineChoices, defDual ? 1 : 0));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_threshold", 1 }, bandName + " Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), defThresh));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_ratio", 1 }, bandName + " Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), defRatio));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_attack", 1 }, bandName + " Attack",
            juce::NormalisableRange<float>(0.05f, 100.0f, 0.05f, 0.3f), defAtt));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_release", 1 }, bandName + " Release",
            juce::NormalisableRange<float>(5.0f, 3000.0f, 1.0f, 0.35f), defRel));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_knee", 1 }, bandName + " Knee",
            juce::NormalisableRange<float>(0.0f, 30.0f, 0.1f), defKnee));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ prefix + "_band_mix", 1 }, bandName + " Band Mix",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));
    };

    // 5 Distinct Bands: Sub (20-90), Low (90-500), Mid (500-1200), Mid-High (1200-5000), Super High (5000-20000)
    addBandParams("sub",      "Sub",      40.0f, 30.0f, 200.0f, true, 30.0f, 4.0f, 10.0f, -18.0f);
    addBandParams("low",      "Low",      35.0f, 20.0f, 150.0f, true, 25.0f, 3.5f, 12.0f, -16.0f);
    addBandParams("mid",      "Mid",      30.0f, 10.0f, 100.0f, true, 20.0f, 3.0f, 12.0f, -16.0f);
    addBandParams("mid_high", "Mid-High", 25.0f,  5.0f,  80.0f, true, 15.0f, 2.8f, 14.0f, -18.0f);
    addBandParams("high",     "Air",      20.0f,  3.0f,  50.0f, true, 15.0f, 2.5f, 16.0f, -18.0f);

    // Master Section
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "input_gain", 1 }, "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "output_gain", 1 }, "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));

    return layout;
}

void MultibandCompressor5BAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    baseSampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    juce::ignoreUnused(samplesPerBlock);

    liveSync.init();

    // Keep reported host latency at 0 for Equalizer APO and realtime playback stability
    setLatencySamples(0);

    subCompressor.setBandType(LA2AOptoCompressor::Band_Sub);
    lowCompressor.setBandType(LA2AOptoCompressor::Band_Low);
    midCompressor.setBandType(LA2AOptoCompressor::Band_Mid);
    midHighCompressor.setBandType(LA2AOptoCompressor::Band_MidHigh);
    highCompressor.setBandType(LA2AOptoCompressor::Band_High);

    // Initialize oversamplers lazily (0ms startup, allocate on-demand)
    oversamplers[0] = nullptr;
    for (int factor = 1; factor <= 4; ++factor)
        oversamplers[factor] = nullptr;

    // Pre-allocate safe buffers
    chunkInputBuffer.setSize(2, CHUNK_SIZE, false, true, false);
    chunkDryBuffer.setSize(2, CHUNK_SIZE, false, true, false);
    subBuffer.setSize(2, MAX_PROC_SAMPLES, false, true, false);
    lowBuffer.setSize(2, MAX_PROC_SAMPLES, false, true, false);
    midBuffer.setSize(2, MAX_PROC_SAMPLES, false, true, false);
    midHighBuffer.setSize(2, MAX_PROC_SAMPLES, false, true, false);
    highBuffer.setSize(2, MAX_PROC_SAMPLES, false, true, false);

    // Prepare primary 1x Linkwitz-Riley crossover
    crossover[0].prepare(baseSampleRate, CHUNK_SIZE);

    // Prepare delay line for dry signal phase alignment
    juce::dsp::ProcessSpec drySpec;
    drySpec.sampleRate = baseSampleRate;
    drySpec.maximumBlockSize = static_cast<juce::uint32>(CHUNK_SIZE);
    drySpec.numChannels = 2;
    dryDelay.prepare(drySpec);
    dryDelay.setDelay(0.0f);

    smoothedF1.reset(baseSampleRate, 0.05);
    smoothedF2.reset(baseSampleRate, 0.05);
    smoothedF3.reset(baseSampleRate, 0.05);
    smoothedF4.reset(baseSampleRate, 0.05);

    float f1 = cached.sub_low_freq->load();
    float f2 = cached.low_mid_freq->load();
    float f3 = cached.mid_high_freq->load();
    float f4 = cached.high_super_freq->load();
    smoothedF1.setCurrentAndTargetValue(f1);
    smoothedF2.setCurrentAndTargetValue(f2);
    smoothedF3.setCurrentAndTargetValue(f3);
    smoothedF4.setCurrentAndTargetValue(f4);

    crossover[0].setCrossoverFrequencies(f1, f2, f3, f4);

    auto* osParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("oversampling"));
    int initialOsIndex = osParam ? osParam->getIndex() : 0;
    currentOversamplingIndex.store(initialOsIndex, std::memory_order_relaxed);
    applyOversamplingRate(initialOsIndex);
}

void MultibandCompressor5BAudioProcessor::releaseResources()
{
    for (int factor = 1; factor <= 4; ++factor)
    {
        if (oversamplers[factor])
            oversamplers[factor]->reset();
    }
    for (int factor = 0; factor <= 4; ++factor)
        crossover[factor].reset();

    subCompressor.reset();
    lowCompressor.reset();
    midCompressor.reset();
    midHighCompressor.reset();
    highCompressor.reset();
    dryDelay.reset();
}

bool MultibandCompressor5BAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void MultibandCompressor5BAudioProcessor::applyOversamplingRate(int factorIndex)
{
    factorIndex = juce::jlimit(0, 4, factorIndex);
    double effSr = baseSampleRate * (1 << factorIndex);
    int latency = 0;

    if (factorIndex > 0)
    {
        if (oversamplers[factorIndex] == nullptr)
        {
            oversamplers[factorIndex] = std::make_unique<juce::dsp::Oversampling<float>>(
                2, factorIndex, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
            oversamplers[factorIndex]->initProcessing(static_cast<size_t>(CHUNK_SIZE));
            crossover[factorIndex].prepare(effSr, CHUNK_SIZE * (1 << factorIndex));
            crossover[factorIndex].setCrossoverFrequencies(
                smoothedF1.getTargetValue(),
                smoothedF2.getTargetValue(),
                smoothedF3.getTargetValue(),
                smoothedF4.getTargetValue()
            );
        }
        latency = static_cast<int>(oversamplers[factorIndex]->getLatencyInSamples());
    }

    currentLatencyReport.store(latency, std::memory_order_relaxed);
    dryDelay.setDelay(static_cast<float>(latency));

    // Smoothly update compressor time constants without resetting audio history or triggering host reload
    subCompressor.setSampleRate(effSr);
    lowCompressor.setSampleRate(effSr);
    midCompressor.setSampleRate(effSr);
    midHighCompressor.setSampleRate(effSr);
    highCompressor.setSampleRate(effSr);
}

void MultibandCompressor5BAudioProcessor::setOversamplingIndex(int index)
{
    index = juce::jlimit(0, 4, index);
    currentOversamplingIndex.store(index, std::memory_order_relaxed);
    applyOversamplingRate(index);

    if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("oversampling")))
    {
        *param = index;
    }
    else if (auto* p = apvts.getParameter("oversampling"))
    {
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(index)));
    }
}

int MultibandCompressor5BAudioProcessor::getCurrentOversamplingIndex() const
{
    return currentOversamplingIndex.load(std::memory_order_relaxed);
}

void MultibandCompressor5BAudioProcessor::publishLiveParameters()
{
    if (isPollingLive.load(std::memory_order_relaxed))
        return;

    float vals[128];
    const auto& params = getParameters();
    int count = juce::jmin(128, params.size());
    for (int i = 0; i < count; ++i)
        vals[i] = params[i]->getValue();
    liveSync.publish(vals, count);
}

void MultibandCompressor5BAudioProcessor::pollLiveParameters()
{
    if (!liveSync.isConnected())
        return;

    float vals[128];
    const auto& params = getParameters();
    int count = juce::jmin(128, params.size());
    if (liveSync.pollUpdate(vals, count))
    {
        isPollingLive.store(true, std::memory_order_relaxed);
        for (int i = 0; i < count; ++i)
        {
            if (std::abs(params[i]->getValue() - vals[i]) > 1e-4f)
            {
                params[i]->setValue(vals[i]);
            }
        }
        isPollingLive.store(false, std::memory_order_relaxed);
    }
}

template <typename SampleType>
void MultibandCompressor5BAudioProcessor::processAudio(juce::AudioBuffer<SampleType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (totalSamples == 0 || numChannels == 0) return;

    pollLiveParameters();

    // Check oversampling factor change
    int osIdx = currentOversamplingIndex.load(std::memory_order_relaxed);
    int targetOsIdx = static_cast<int>(cached.oversampling->load());
    if (targetOsIdx != osIdx)
    {
        osIdx = juce::jlimit(0, 4, targetOsIdx);
        currentOversamplingIndex.store(osIdx, std::memory_order_relaxed);
        applyOversamplingRate(osIdx);
    }

    // Input metering & Gain
    float inGainDb = cached.input_gain->load();
    float inGainLin = juce::Decibels::decibelsToGain(inGainDb);
    float inPeak = static_cast<float>(buffer.getMagnitude(0, totalSamples));
    inputLevel.store(juce::Decibels::gainToDecibels(juce::jmax(1e-6f, inPeak * inGainLin)), std::memory_order_relaxed);

    float outGainDb = cached.output_gain->load();
    float outGainLin = juce::Decibels::decibelsToGain(outGainDb);
    float masterMix = cached.mix->load() / 100.0f;
    bool isAdvance = cached.ui_mode->load() > 0.5f;

    // Solo flags
    bool subSolo = cached.sub_solo->load() > 0.5f;
    bool lowSolo = cached.low_solo->load() > 0.5f;
    bool midSolo = cached.mid_solo->load() > 0.5f;
    bool midHighSolo = cached.mid_high_solo->load() > 0.5f;
    bool highSolo = cached.high_solo->load() > 0.5f;
    bool anySolo = subSolo || lowSolo || midSolo || midHighSolo || highSolo;

    // Setup compressors before chunk loop
    auto configComp = [&](LA2AOptoCompressor& comp, const CachedParams::BandParams& bp)
    {
        comp.setAdvanceMode(isAdvance);
        comp.setMakeupGainDb(bp.gain->load());
        comp.setTubeWarmth(bp.warmth->load());
        comp.setBypass(bp.bypass->load() > 0.5f);
        comp.setMuted(bp.mute->load() > 0.5f);

        if (!isAdvance)
        {
            comp.setPeakReduction(bp.peak_reduction->load());
            comp.setLimitMode(bp.mode->load() > 0.5f);
        }
        else
        {
            int engIdx = static_cast<int>(bp.engine->load());
            comp.setEngineType(engIdx == 0 ? LA2AOptoCompressor::Engine_Single : LA2AOptoCompressor::Engine_Dual);

            comp.setThresholdDb(bp.threshold->load());
            comp.setRatio(bp.ratio->load());
            comp.setAttackMs(bp.attack->load());
            comp.setReleaseMs(bp.release->load());
            comp.setKneeDb(bp.knee->load());
            comp.setBandMix(bp.band_mix->load());
        }
    };

    configComp(subCompressor, cached.sub);
    configComp(lowCompressor, cached.low);
    configComp(midCompressor, cached.mid);
    configComp(midHighCompressor, cached.mid_high);
    configComp(highCompressor, cached.high);

    // Process in safe fixed-size chunks (avoids memory allocation and buffer overflow)
    for (int offset = 0; offset < totalSamples; offset += CHUNK_SIZE)
    {
        const int chunkSize = std::min(CHUNK_SIZE, totalSamples - offset);

        // 1. Copy from host buffer to chunkInputBuffer (with input gain applied, duplicate mono to stereo if needed)
        for (int ch = 0; ch < 2; ++ch)
        {
            int srcCh = std::min(ch, numChannels - 1);
            const SampleType* src = buffer.getReadPointer(srcCh, offset);
            float* dst = chunkInputBuffer.getWritePointer(ch);
            for (int i = 0; i < chunkSize; ++i)
                dst[i] = static_cast<float>(src[i]) * inGainLin;
        }

        // 2. Save dry if mix < 1.0f and align latency
        if (masterMix < 0.999f)
        {
            for (int ch = 0; ch < 2; ++ch)
                chunkDryBuffer.copyFrom(ch, 0, chunkInputBuffer.getReadPointer(ch), chunkSize);

            juce::dsp::AudioBlock<float> dryBlock(chunkDryBuffer.getArrayOfWritePointers(), 2, static_cast<size_t>(chunkSize));
            juce::dsp::ProcessContextReplacing<float> dryCtx(dryBlock);
            dryDelay.process(dryCtx);
        }

        // 3. Prepare audio block & oversample
        float* chunkPointers[2] = { chunkInputBuffer.getWritePointer(0), chunkInputBuffer.getWritePointer(1) };
        juce::dsp::AudioBlock<float> baseBlock(chunkPointers, 2, static_cast<size_t>(chunkSize));
        juce::dsp::AudioBlock<float> procBlock = baseBlock;

        if (osIdx > 0 && osIdx <= 4 && oversamplers[osIdx])
            procBlock = oversamplers[osIdx]->processSamplesUp(baseBlock);

        const int procSamples = static_cast<int>(procBlock.getNumSamples());

        // 4. Update smoothed crossover points
        float f1 = cached.sub_low_freq->load();
        float f2 = cached.low_mid_freq->load();
        float f3 = cached.mid_high_freq->load();
        float f4 = cached.high_super_freq->load();
        smoothedF1.setTargetValue(f1);
        smoothedF2.setTargetValue(f2);
        smoothedF3.setTargetValue(f3);
        smoothedF4.setTargetValue(f4);

        float sf1 = smoothedF1.skip(chunkSize);
        float sf2 = smoothedF2.skip(chunkSize);
        float sf3 = smoothedF3.skip(chunkSize);
        float sf4 = smoothedF4.skip(chunkSize);

        crossover[osIdx].setCrossoverFrequencies(sf1, sf2, sf3, sf4);

        // 5. Split into 5 Bands
        float* bandPointers[5][2] = {
            { subBuffer.getWritePointer(0), subBuffer.getWritePointer(1) },
            { lowBuffer.getWritePointer(0), lowBuffer.getWritePointer(1) },
            { midBuffer.getWritePointer(0), midBuffer.getWritePointer(1) },
            { midHighBuffer.getWritePointer(0), midHighBuffer.getWritePointer(1) },
            { highBuffer.getWritePointer(0), highBuffer.getWritePointer(1) }
        };

        const float* inPointers[2] = { procBlock.getChannelPointer(0), procBlock.getChannelPointer(1) };
        crossover[osIdx].process(inPointers,
                                 bandPointers[0],
                                 bandPointers[1],
                                 bandPointers[2],
                                 bandPointers[3],
                                 bandPointers[4],
                                 procSamples);

        // 6. Process each band compressor
        subCompressor.process(bandPointers[0], 2, procSamples);
        lowCompressor.process(bandPointers[1], 2, procSamples);
        midCompressor.process(bandPointers[2], 2, procSamples);
        midHighCompressor.process(bandPointers[3], 2, procSamples);
        highCompressor.process(bandPointers[4], 2, procSamples);

        // 7. Sum bands back into procBlock
        for (int ch = 0; ch < 2; ++ch)
        {
            float* dest = procBlock.getChannelPointer(ch);
            juce::FloatVectorOperations::clear(dest, procSamples);

            if (!anySolo || subSolo)
                juce::FloatVectorOperations::add(dest, subBuffer.getReadPointer(ch), procSamples);
            if (!anySolo || lowSolo)
                juce::FloatVectorOperations::add(dest, lowBuffer.getReadPointer(ch), procSamples);
            if (!anySolo || midSolo)
                juce::FloatVectorOperations::add(dest, midBuffer.getReadPointer(ch), procSamples);
            if (!anySolo || midHighSolo)
                juce::FloatVectorOperations::add(dest, midHighBuffer.getReadPointer(ch), procSamples);
            if (!anySolo || highSolo)
                juce::FloatVectorOperations::add(dest, highBuffer.getReadPointer(ch), procSamples);
        }

        // 8. Downsample if oversampling was active
        if (osIdx > 0 && osIdx <= 4 && oversamplers[osIdx])
            oversamplers[osIdx]->processSamplesDown(baseBlock);

        // 9. Master Dry / Wet Mix
        if (masterMix < 0.999f)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                float* wetData = chunkInputBuffer.getWritePointer(ch);
                const float* dryData = chunkDryBuffer.getReadPointer(ch);
                for (int i = 0; i < chunkSize; ++i)
                    wetData[i] = dryData[i] * (1.0f - masterMix) + wetData[i] * masterMix;
            }
        }

        // 10. Copy back to host buffer with output gain applied (support mono & stereo, float & double!)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            int srcCh = std::min(ch, 1);
            const float* src = chunkInputBuffer.getReadPointer(srcCh);
            SampleType* dst = buffer.getWritePointer(ch, offset);
            for (int i = 0; i < chunkSize; ++i)
                dst[i] = static_cast<SampleType>(src[i] * outGainLin);
        }
    }

    // Output Metering
    float outPeak = static_cast<float>(buffer.getMagnitude(0, totalSamples));
    outputLevel.store(juce::Decibels::gainToDecibels(juce::jmax(1e-6f, outPeak)), std::memory_order_relaxed);
}

void MultibandCompressor5BAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    processAudio(buffer);
}

void MultibandCompressor5BAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    processAudio(buffer);
}

juce::AudioProcessorEditor* MultibandCompressor5BAudioProcessor::createEditor()
{
    return new MultibandCompressor5BAudioProcessorEditor(*this);
}

void MultibandCompressor5BAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MultibandCompressor5BAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        if (auto* osParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("oversampling")))
        {
            int idx = osParam->getIndex();
            currentOversamplingIndex.store(idx, std::memory_order_relaxed);
            applyOversamplingRate(idx);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultibandCompressor5BAudioProcessor();
}
