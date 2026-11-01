#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "YamahaLookAndFeel.h"
#include "CrossoverDisplay.h"
#include "CompressorTransferGraph.h"
#include "SettingsPanel.h"
#include "MasterMeterPanel.h"

// Master Level Meter Component
class YamahaMeter : public juce::Component
{
public:
    YamahaMeter(const juce::String& title, juce::Colour accentCol)
        : label(title), accent(accentCol) {}

    void setLevel(float valDb)
    {
        if (std::isnan(valDb) || std::isinf(valDb)) valDb = -100.0f;
        currentDb = juce::jlimit(-100.0f, 24.0f, valDb);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() < 30.0f || bounds.getHeight() < 10.0f)
            return;

        // Dark background
        g.setColour(juce::Colour(0xff121417));
        g.fillRoundedRectangle(bounds, 3.0f);

        // Frame
        g.setColour(juce::Colour(0xff2d333f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        auto meterBar = bounds.reduced(2.0f);
        if (meterBar.getWidth() > 2.0f && meterBar.getHeight() > 2.0f)
        {
            float norm = juce::jlimit(0.0f, 1.0f, (currentDb + 60.0f) / 66.0f);
            auto activeBar = meterBar.withWidth(juce::jmax(0.0f, meterBar.getWidth() * norm));
            g.setColour(accent);
            g.fillRoundedRectangle(activeBar, 2.0f);
        }

        auto textArea = bounds.reduced(4.0f, 0.0f);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xffb5c0d0));
        g.drawText(label, textArea.removeFromLeft(textArea.getWidth() * 0.45f).toNearestInt(), juce::Justification::centredLeft, false);

        juce::String valStr = juce::String(static_cast<int>(currentDb)) + " dB";
        g.drawText(valStr, textArea.toNearestInt(), juce::Justification::centredRight, false);
    }

private:
    juce::String label;
    juce::Colour accent;
    float currentDb = 0.0f;
};

class MultibandCompressor5BAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  public juce::Timer
{
public:
    MultibandCompressor5BAudioProcessorEditor(MultibandCompressor5BAudioProcessor&);
    ~MultibandCompressor5BAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent&) override;
    void parentHierarchyChanged() override;

private:
    MultibandCompressor5BAudioProcessor& audioProcessor;
    YamahaLookAndFeel yamahaLF;

    // Header Mode Switch & Reset to Default
    juce::ToggleButton modeToggleButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeToggleAttach;
    juce::TextButton resetDefaultButton;

    // Header Preset Manager Controls
    juce::Label presetLabel;
    juce::ComboBox presetComboBox;
    juce::TextButton savePresetButton;
    juce::TextButton loadPresetButton;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void updatePresetList(const juce::String& presetToSelect = {});

    // Crossover Interactive Display (5 Bands, 4 Splitters)
    CrossoverDisplay crossoverDisplay;

    // Helper struct for Band UI Strip
    struct BandStripControls
    {
        juce::Label titleLabel;
        juce::Label freqRangeLabel;

        // Yamaha dspMixFx Compression Curve Graph with "T", "R", and vertical GR meter
        std::unique_ptr<CompressorTransferGraph> transferGraph;

        // Common Knobs
        juce::Slider gainSlider;
        juce::Label gainLabel;
        juce::Slider warmthSlider;
        juce::Label warmthLabel;

        juce::ToggleButton soloButton;
        juce::ToggleButton muteButton;
        juce::ToggleButton bypassButton;

        // Normal Mode Controls
        juce::Slider peakReductionSlider;
        juce::Label peakReductionLabel;
        juce::ToggleButton modeButton; // Comp vs Limit

        // Advance Mode Controls
        juce::ToggleButton engineButton; // Single vs Dual Stepper
        juce::Slider thresholdSlider;
        juce::Label thresholdLabel;
        juce::Slider ratioSlider;
        juce::Label ratioLabel;
        juce::Slider attackSlider;
        juce::Label attackLabel;
        juce::Slider releaseSlider;
        juce::Label releaseLabel;
        juce::Slider kneeSlider;
        juce::Label kneeLabel;
        juce::Slider bandMixSlider;
        juce::Label bandMixLabel;

        // Attachments
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakReductionAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> engineAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kneeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandMixAttach;

        void setAdvanceVisibility(bool isAdvance)
        {
            // Normal only
            peakReductionSlider.setVisible(!isAdvance);
            peakReductionLabel.setVisible(!isAdvance);
            modeButton.setVisible(!isAdvance);

            // Advance only
            engineButton.setVisible(isAdvance);
            thresholdSlider.setVisible(isAdvance);
            thresholdLabel.setVisible(isAdvance);
            ratioSlider.setVisible(isAdvance);
            ratioLabel.setVisible(isAdvance);
            attackSlider.setVisible(isAdvance);
            attackLabel.setVisible(isAdvance);
            releaseSlider.setVisible(isAdvance);
            releaseLabel.setVisible(isAdvance);
            kneeSlider.setVisible(isAdvance);
            kneeLabel.setVisible(isAdvance);
            bandMixSlider.setVisible(isAdvance);
            bandMixLabel.setVisible(isAdvance);
        }
    };

    BandStripControls subBand;
    BandStripControls lowBand;
    BandStripControls midBand;
    BandStripControls midHighBand;
    BandStripControls highBand;

    void setupBandStrip(BandStripControls& strip, const juce::String& title,
                        const juce::String& prefix, juce::Colour accentColour);

    // Master Controls
    juce::Label masterTitleLabel;
    juce::Label masterSubLabel;
    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;
    juce::Slider mixSlider;
    juce::Label mixLabel;

    std::unique_ptr<MasterMeterPanel> masterMeterPanel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach;

    // Bottom Footer (Oversampling Utility Bar)
    juce::Label oversamplingTitleLabel;
    juce::TextButton osButtons[5];
    juce::Label latencyStatusLabel;

    void updateOversamplingButtons(int activeIndex);

    // Settings & Hardware Configuration
    GearButton gearButton;
    std::unique_ptr<SettingsPanel> settingsPanel;

    // GPU Acceleration Context (Stubbed for 100% stable DirectWrite / GDI software rendering in Eq APO & DAWs)
    void setGpuAcceleration(bool enabled) { juce::ignoreUnused(enabled); }

    // Cached frequencies to eliminate redundant string allocations in timerCallback
    float lastFreq1 = -1.0f, lastFreq2 = -1.0f, lastFreq3 = -1.0f, lastFreq4 = -1.0f;

    bool lastKnownAdvanceMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultibandCompressor5BAudioProcessorEditor)
};
