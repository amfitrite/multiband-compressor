#include "PluginProcessor.h"
#include "PluginEditor.h"

MultibandCompressorAudioProcessor::MultibandCompressorAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

MultibandCompressorAudioProcessor::~MultibandCompressorAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MultibandCompressorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Crossover frequencies
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "low_mid_freq", 1 }, "Low-Mid Crossover",
        juce::NormalisableRange<float>(40.0f, 1000.0f, 1.0f, 0.4f), 250.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mid_high_freq", 1 }, "Mid-High Crossover",
        juce::NormalisableRange<float>(1000.0f, 16000.0f, 1.0f, 0.4f), 3500.0f));

    // LOW BAND
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "low_peak_reduction", 1 }, "Low Peak Reduction",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "low_gain", 1 }, "Low Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "low_mode", 1 }, "Low Limit Mode", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "low_warmth", 1 }, "Low Tube Warmth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "low_bypass", 1 }, "Low Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "low_solo", 1 }, "Low Solo", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "low_mute", 1 }, "Low Mute", false));

    // MID BAND
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mid_peak_reduction", 1 }, "Mid Peak Reduction",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mid_gain", 1 }, "Mid Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "mid_mode", 1 }, "Mid Limit Mode", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mid_warmth", 1 }, "Mid Tube Warmth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "mid_bypass", 1 }, "Mid Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "mid_solo", 1 }, "Mid Solo", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "mid_mute", 1 }, "Mid Mute", false));

    // HIGH BAND
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "high_peak_reduction", 1 }, "High Peak Reduction",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "high_gain", 1 }, "High Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "high_mode", 1 }, "High Limit Mode", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "high_warmth", 1 }, "High Tube Warmth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "high_bypass", 1 }, "High Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "high_solo", 1 }, "High Solo", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "high_mute", 1 }, "High Mute", false));

    // MASTER SECTION
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

void MultibandCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    crossover.prepare(sampleRate, samplesPerBlock);
    lowCompressor.prepare(sampleRate);
    midCompressor.prepare(sampleRate);
    highCompressor.prepare(sampleRate);

    lowBuffer.setSize(2, samplesPerBlock);
    midBuffer.setSize(2, samplesPerBlock);
    highBuffer.setSize(2, samplesPerBlock);
    dryBuffer.setSize(2, samplesPerBlock);

    smoothedLowFreq.reset(sampleRate, 0.05);
    smoothedHighFreq.reset(sampleRate, 0.05);

    auto* lowParam = apvts.getRawParameterValue("low_mid_freq");
    auto* highParam = apvts.getRawParameterValue("mid_high_freq");
    smoothedLowFreq.setCurrentAndTargetValue(lowParam ? lowParam->load() : 250.0f);
    smoothedHighFreq.setCurrentAndTargetValue(highParam ? highParam->load() : 3500.0f);
}

void MultibandCompressorAudioProcessor::releaseResources()
{
}

bool MultibandCompressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void MultibandCompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (totalNumInputChannels < 2)
        return; // requires stereo processing

    // Input gain & input metering
    const float inGainDb = apvts.getRawParameterValue("input_gain")->load();
    const float inGainLinear = juce::Decibels::decibelsToGain(inGainDb);
    buffer.applyGain(inGainLinear);

    float inPeak = buffer.getMagnitude(0, numSamples);
    inputLevel.store(inPeak > 1e-5f ? juce::Decibels::gainToDecibels(inPeak) : -100.0f, std::memory_order_relaxed);

    // Save dry buffer for mix
    dryBuffer.setSize(2, numSamples, false, false, true);
    for (int ch = 0; ch < 2; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Update crossover frequencies with smoothing
    const float targetLowFreq = apvts.getRawParameterValue("low_mid_freq")->load();
    const float targetHighFreq = apvts.getRawParameterValue("mid_high_freq")->load();
    smoothedLowFreq.setTargetValue(targetLowFreq);
    smoothedHighFreq.setTargetValue(targetHighFreq);

    crossover.setCrossoverFrequencies(smoothedLowFreq.getNextValue(), smoothedHighFreq.getNextValue());

    // Prepare split band buffers
    lowBuffer.setSize(2, numSamples, false, false, true);
    midBuffer.setSize(2, numSamples, false, false, true);
    highBuffer.setSize(2, numSamples, false, false, true);

    const float* const inChannels[2] = { buffer.getReadPointer(0), buffer.getReadPointer(1) };
    float* const lowChannels[2] = { lowBuffer.getWritePointer(0), lowBuffer.getWritePointer(1) };
    float* const midChannels[2] = { midBuffer.getWritePointer(0), midBuffer.getWritePointer(1) };
    float* const highChannels[2] = { highBuffer.getWritePointer(0), highBuffer.getWritePointer(1) };

    // 4th order Linkwitz-Riley crossover filtering
    crossover.process(inChannels, lowChannels, midChannels, highChannels, numSamples);

    // Determine Solo / Mute state across bands
    const bool lowSolo = apvts.getRawParameterValue("low_solo")->load() > 0.5f;
    const bool midSolo = apvts.getRawParameterValue("mid_solo")->load() > 0.5f;
    const bool highSolo = apvts.getRawParameterValue("high_solo")->load() > 0.5f;
    const bool anySolo = lowSolo || midSolo || highSolo;

    const bool lowMute = apvts.getRawParameterValue("low_mute")->load() > 0.5f;
    const bool midMute = apvts.getRawParameterValue("mid_mute")->load() > 0.5f;
    const bool highMute = apvts.getRawParameterValue("high_mute")->load() > 0.5f;

    const bool lowEffectiveMute = anySolo ? !lowSolo : lowMute;
    const bool midEffectiveMute = anySolo ? !midSolo : midMute;
    const bool highEffectiveMute = anySolo ? !highSolo : highMute;

    // Configure Low Compressor
    lowCompressor.setPeakReduction(apvts.getRawParameterValue("low_peak_reduction")->load());
    lowCompressor.setMakeupGainDb(apvts.getRawParameterValue("low_gain")->load());
    lowCompressor.setLimitMode(apvts.getRawParameterValue("low_mode")->load() > 0.5f);
    lowCompressor.setTubeWarmth(apvts.getRawParameterValue("low_warmth")->load());
    lowCompressor.setBypass(apvts.getRawParameterValue("low_bypass")->load() > 0.5f);
    lowCompressor.setMuted(lowEffectiveMute);

    // Configure Mid Compressor
    midCompressor.setPeakReduction(apvts.getRawParameterValue("mid_peak_reduction")->load());
    midCompressor.setMakeupGainDb(apvts.getRawParameterValue("mid_gain")->load());
    midCompressor.setLimitMode(apvts.getRawParameterValue("mid_mode")->load() > 0.5f);
    midCompressor.setTubeWarmth(apvts.getRawParameterValue("mid_warmth")->load());
    midCompressor.setBypass(apvts.getRawParameterValue("mid_bypass")->load() > 0.5f);
    midCompressor.setMuted(midEffectiveMute);

    // Configure High Compressor
    highCompressor.setPeakReduction(apvts.getRawParameterValue("high_peak_reduction")->load());
    highCompressor.setMakeupGainDb(apvts.getRawParameterValue("high_gain")->load());
    highCompressor.setLimitMode(apvts.getRawParameterValue("high_mode")->load() > 0.5f);
    highCompressor.setTubeWarmth(apvts.getRawParameterValue("high_warmth")->load());
    highCompressor.setBypass(apvts.getRawParameterValue("high_bypass")->load() > 0.5f);
    highCompressor.setMuted(highEffectiveMute);

    // Process each band through its LA-2A opto compressor
    lowCompressor.process(lowChannels, 2, numSamples);
    midCompressor.process(midChannels, 2, numSamples);
    highCompressor.process(highChannels, 2, numSamples);

    // Recombine bands: Output = Low + Mid + High
    buffer.clear();
    for (int ch = 0; ch < 2; ++ch)
    {
        buffer.addFrom(ch, 0, lowBuffer, ch, 0, numSamples);
        buffer.addFrom(ch, 0, midBuffer, ch, 0, numSamples);
        buffer.addFrom(ch, 0, highBuffer, ch, 0, numSamples);
    }

    // Apply Dry/Wet mix
    const float mixPct = apvts.getRawParameterValue("mix")->load() / 100.0f;
    if (mixPct < 0.999f)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            float* outData = buffer.getWritePointer(ch);
            const float* dryData = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                outData[i] = dryData[i] * (1.0f - mixPct) + outData[i] * mixPct;
        }
    }

    // Master Output Gain
    const float outGainDb = apvts.getRawParameterValue("output_gain")->load();
    const float outGainLinear = juce::Decibels::decibelsToGain(outGainDb);
    buffer.applyGain(outGainLinear);

    // Output level metering
    float outPeak = buffer.getMagnitude(0, numSamples);
    outputLevel.store(outPeak > 1e-5f ? juce::Decibels::gainToDecibels(outPeak) : -100.0f, std::memory_order_relaxed);
}

void MultibandCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MultibandCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorEditor* MultibandCompressorAudioProcessor::createEditor()
{
    return new MultibandCompressorAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultibandCompressorAudioProcessor();
}
