#include "PluginProcessor.h"
#include "PluginEditor.h"

MultibandCompressor5BAudioProcessorEditor::MultibandCompressor5BAudioProcessorEditor(MultibandCompressor5BAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), crossoverDisplay(p.getAPVTS())
{
    setLookAndFeel(&yamahaLF);

    // Tooltip Window styling
    tooltipWindow.setMillisecondsBeforeTipAppears(700);

    // Header Mode Switch Button
    modeToggleButton.setButtonText("MODE: NORMAL");
    modeToggleButton.setTooltip("Toggle between Normal (Optical) and Advance (Precision) modes");
    modeToggleButton.getProperties().set("btnColour", static_cast<int>(juce::Colour(0xff00d2d3).getARGB()));
    modeToggleButton.onClick = [this]
    {
        bool isAdv = modeToggleButton.getToggleState();
        modeToggleButton.setButtonText(isAdv ? "MODE: ADVANCE" : "MODE: NORMAL");
        resized();
    };
    addAndMakeVisible(modeToggleButton);

    modeToggleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "ui_mode", modeToggleButton);

    // Reset to Default Button
    resetDefaultButton.setButtonText("RESET DEFAULT");
    resetDefaultButton.setTooltip("Reset all parameters to their default states");
    resetDefaultButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252930));
    resetDefaultButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe17055));
    resetDefaultButton.onClick = [this]
    {
        audioProcessor.resetToDefault();
        updateOversamplingButtons(0);
        modeToggleButton.setToggleState(false, juce::dontSendNotification);
        modeToggleButton.setButtonText("MODE: NORMAL");
        presetComboBox.setSelectedId(0, juce::dontSendNotification);
        presetComboBox.setTextWhenNothingSelected("Select Preset...");
        resized();
        audioProcessor.publishLiveParameters();
    };
    addAndMakeVisible(resetDefaultButton);

    // Preset Label
    presetLabel.setText("PRESET:", juce::dontSendNotification);
    presetLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    presetLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    // Preset ComboBox
    presetComboBox.setTextWhenNothingSelected("Select Preset...");
    presetComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff20242c));
    presetComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe1e8f0));
    presetComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3d4756));
    presetComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff00d2d3));
    presetComboBox.onChange = [this]
    {
        auto selectedName = presetComboBox.getText();
        if (selectedName.isNotEmpty() && selectedName != "Select Preset...")
        {
            if (audioProcessor.getPresetManager().loadPreset(selectedName))
            {
                audioProcessor.publishLiveParameters();
                bool isAdv = (audioProcessor.getAPVTS().getRawParameterValue("ui_mode")->load() > 0.5f);
                modeToggleButton.setToggleState(isAdv, juce::dontSendNotification);
                modeToggleButton.setButtonText(isAdv ? "MODE: ADVANCE" : "MODE: NORMAL");
                resized();
            }
        }
    };
    addAndMakeVisible(presetComboBox);

    // Save Preset Button
    savePresetButton.setButtonText("SAVE");
    savePresetButton.setTooltip("Save current state as a new preset");
    savePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e3799).withAlpha(0.6f));
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff70a1ff));
    savePresetButton.onClick = [this]
    {
        auto* w = new juce::AlertWindow("Save Preset", "Enter preset name:", juce::AlertWindow::NoIcon);
        w->addTextEditor("name", "", "Preset Name:");
        w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int result)
        {
            if (result == 1)
            {
                auto name = w->getTextEditorContents("name").trim();
                if (name.isNotEmpty())
                {
                    audioProcessor.getPresetManager().savePreset(name);
                    updatePresetList(name);
                }
            }
        }), true);
    };
    addAndMakeVisible(savePresetButton);

    // Load Preset Button
    loadPresetButton.setButtonText("LOAD");
    loadPresetButton.setTooltip("Load an external preset file (.json)");
    loadPresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252930));
    loadPresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffa4b0be));
    loadPresetButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load Preset File",
            audioProcessor.getPresetManager().getPresetsDirectory(),
            "*.json");

        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                if (audioProcessor.getPresetManager().loadPresetFromFile(file))
                {
                    audioProcessor.publishLiveParameters();
                    updatePresetList(file.getFileNameWithoutExtension());
                    bool isAdv = (audioProcessor.getAPVTS().getRawParameterValue("ui_mode")->load() > 0.5f);
                    modeToggleButton.setToggleState(isAdv, juce::dontSendNotification);
                    modeToggleButton.setButtonText(isAdv ? "MODE: ADVANCE" : "MODE: NORMAL");
                    resized();
                }
            }
        });
    };
    addAndMakeVisible(loadPresetButton);

    // Populate preset dropdown
    updatePresetList();

    // Crossover Interactive Display
    addAndMakeVisible(crossoverDisplay);

    // Setup 5 Bands
    setupBandStrip(subBand,     "SUB",      "sub",      juce::Colour(0xffe67e22));
    setupBandStrip(lowBand,     "LOW",      "low",      juce::Colour(0xfff1c40f));
    setupBandStrip(midBand,     "MID",      "mid",      juce::Colour(0xff00d2d3));
    setupBandStrip(midHighBand, "MID-HIGH", "mid_high", juce::Colour(0xff3b82f6));
    setupBandStrip(highBand,    "AIR",      "high",     juce::Colour(0xffb05bf5));

    // Master Controls
    masterTitleLabel.setText("MASTER", juce::dontSendNotification);
    masterTitleLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    masterTitleLabel.setJustificationType(juce::Justification::centred);
    masterTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4cd137));
    addAndMakeVisible(masterTitleLabel);

    masterSubLabel.setText("OUTPUT BUS", juce::dontSendNotification);
    masterSubLabel.setFont(juce::FontOptions(10.0f));
    masterSubLabel.setJustificationType(juce::Justification::centred);
    masterSubLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
    addAndMakeVisible(masterSubLabel);

    auto setupMasterSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& suffix)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
        s.setTextValueSuffix(suffix);
        s.setMouseDragSensitivity(260);
        s.getProperties().set("accentColour", static_cast<int>(juce::Colour(0xff4cd137).getARGB()));
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colour(0xffc8d6e5));
        addAndMakeVisible(l);
    };

    setupMasterSlider(inputGainSlider, inputGainLabel, "IN GAIN", " dB");
    inputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "input_gain", inputGainSlider);

    setupMasterSlider(outputGainSlider, outputGainLabel, "OUT GAIN", " dB");
    outputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "output_gain", outputGainSlider);

    setupMasterSlider(mixSlider, mixLabel, "MASTER MIX", " %");
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixSlider);

    masterMeterPanel = std::make_unique<MasterMeterPanel>();
    addAndMakeVisible(*masterMeterPanel);

    // Bottom Footer (Oversampling Bar)
    oversamplingTitleLabel.setText("OVERSAMPLING:", juce::dontSendNotification);
    oversamplingTitleLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    oversamplingTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff707b8c));
    addAndMakeVisible(oversamplingTitleLabel);

    juce::String osLabels[5] = { "1x (OFF)", "2x", "4x", "8x", "16x" };
    for (int i = 0; i < 5; ++i)
    {
        osButtons[i].setButtonText(osLabels[i]);
        osButtons[i].setClickingTogglesState(false);
        osButtons[i].onClick = [this, i]
        {
            audioProcessor.setOversamplingIndex(i);
            updateOversamplingButtons(i);
        };
        addAndMakeVisible(osButtons[i]);
    }

    latencyStatusLabel.setFont(juce::FontOptions(11.0f));
    latencyStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff707b8c));
    latencyStatusLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(latencyStatusLabel);

    // Settings & Hardware Configuration (Gear Button & Popover)
    gearButton.onClick = [this]
    {
        bool show = gearButton.getToggleState();
        if (settingsPanel != nullptr)
        {
            settingsPanel->setVisible(show);
            if (show)
                settingsPanel->toFront(true);
        }
    };
    addAndMakeVisible(gearButton);

    settingsPanel = std::make_unique<SettingsPanel>(audioProcessor.getAPVTS());
    settingsPanel->onClose = [this]
    {
        if (settingsPanel != nullptr)
            settingsPanel->setVisible(false);
        gearButton.setToggleState(false, juce::dontSendNotification);
    };
    settingsPanel->onGpuAccelerationChanged = [this](bool enabled)
    {
        setGpuAcceleration(enabled);
    };
    addChildComponent(*settingsPanel); // Initially hidden

    setResizable(true, true);
    setResizeLimits(900, 700, 1920, 1400);
    setSize(1280, 920);
    audioProcessor.publishLiveParameters();
    startTimerHz(30);
}

MultibandCompressor5BAudioProcessorEditor::~MultibandCompressor5BAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void MultibandCompressor5BAudioProcessorEditor::parentHierarchyChanged()
{
    juce::AudioProcessorEditor::parentHierarchyChanged();
}

void MultibandCompressor5BAudioProcessorEditor::setupBandStrip(
    BandStripControls& strip, const juce::String& title,
    const juce::String& prefix, juce::Colour accentColour)
{
    strip.titleLabel.setText(title, juce::dontSendNotification);
    strip.titleLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    strip.titleLabel.setJustificationType(juce::Justification::centred);
    strip.titleLabel.setColour(juce::Label::textColourId, accentColour);
    addAndMakeVisible(strip.titleLabel);

    strip.freqRangeLabel.setText("...", juce::dontSendNotification);
    strip.freqRangeLabel.setFont(juce::FontOptions(10.0f));
    strip.freqRangeLabel.setJustificationType(juce::Justification::centred);
    strip.freqRangeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
    addAndMakeVisible(strip.freqRangeLabel);

    // Yamaha dspMixFx Compression Curve Graph
    strip.transferGraph = std::make_unique<CompressorTransferGraph>(accentColour, [this, prefix](float newThreshold)
    {
        const bool isAdv = modeToggleButton.getToggleState();
        if (isAdv)
        {
            if (auto* param = audioProcessor.getAPVTS().getParameter(prefix + "_threshold"))
                param->setValueNotifyingHost(param->convertTo0to1(newThreshold));
        }
        else
        {
            float newPr = juce::jlimit(0.0f, 100.0f, (6.0f - newThreshold) / 0.52f);
            if (auto* param = audioProcessor.getAPVTS().getParameter(prefix + "_peak_reduction"))
                param->setValueNotifyingHost(param->convertTo0to1(newPr));
        }
    });
    addAndMakeVisible(*strip.transferGraph);

    auto initKnob = [this, accentColour](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& suffix)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 16);
        s.setTextValueSuffix(suffix);
        s.setMouseDragSensitivity(260);
        s.getProperties().set("accentColour", static_cast<int>(accentColour.getARGB()));
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colour(0xffc8d6e5));
        addAndMakeVisible(l);
    };

    initKnob(strip.gainSlider, strip.gainLabel, "GAIN", " dB");
    strip.gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_gain", strip.gainSlider);

    initKnob(strip.warmthSlider, strip.warmthLabel, "WARMTH", " %");
    strip.warmthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_warmth", strip.warmthSlider);

    // Buttons
    auto setupBtn = [this](juce::ToggleButton& b, const juce::String& text, juce::Colour col)
    {
        b.setButtonText(text);
        b.getProperties().set("btnColour", static_cast<int>(col.getARGB()));
        addAndMakeVisible(b);
    };

    setupBtn(strip.soloButton, "SOLO", juce::Colour(0xfff1c40f));
    strip.soloAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), prefix + "_solo", strip.soloButton);
    strip.soloButton.onClick = [this] { audioProcessor.publishLiveParameters(); };

    setupBtn(strip.muteButton, "MUTE", juce::Colour(0xffe74c3c));
    strip.muteAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), prefix + "_mute", strip.muteButton);
    strip.muteButton.onClick = [this] { audioProcessor.publishLiveParameters(); };

    setupBtn(strip.bypassButton, "BYPASS", juce::Colour(0xff707b8c));
    strip.bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), prefix + "_bypass", strip.bypassButton);
    strip.bypassButton.onClick = [this] { audioProcessor.publishLiveParameters(); };

    // Normal Mode
    initKnob(strip.peakReductionSlider, strip.peakReductionLabel, "REDUCTION", "");
    strip.peakReductionAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_peak_reduction", strip.peakReductionSlider);

    setupBtn(strip.modeButton, "COMP", accentColour);
    strip.modeButton.onClick = [&strip]
    {
        strip.modeButton.setButtonText(strip.modeButton.getToggleState() ? "LIMIT" : "COMP");
    };
    strip.modeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), prefix + "_mode", strip.modeButton);

    // Advance Mode
    setupBtn(strip.engineButton, "DUAL OPTO", accentColour);
    strip.engineButton.onClick = [&strip]
    {
        strip.engineButton.setButtonText(strip.engineButton.getToggleState() ? "DUAL OPTO" : "VCA PREC");
    };
    strip.engineAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), prefix + "_engine", strip.engineButton);

    initKnob(strip.thresholdSlider, strip.thresholdLabel, "THRESH", " dB");
    strip.thresholdAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_threshold", strip.thresholdSlider);

    initKnob(strip.ratioSlider, strip.ratioLabel, "RATIO", ":1");
    strip.ratioAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_ratio", strip.ratioSlider);

    initKnob(strip.attackSlider, strip.attackLabel, "ATTACK", " ms");
    strip.attackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_attack", strip.attackSlider);

    initKnob(strip.releaseSlider, strip.releaseLabel, "RELEASE", " ms");
    strip.releaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_release", strip.releaseSlider);

    initKnob(strip.kneeSlider, strip.kneeLabel, "KNEE", " dB");
    strip.kneeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_knee", strip.kneeSlider);

    initKnob(strip.bandMixSlider, strip.bandMixLabel, "BAND MIX", " %");
    strip.bandMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), prefix + "_band_mix", strip.bandMixSlider);

    bool isAdv = (audioProcessor.getAPVTS().getRawParameterValue("ui_mode")->load() > 0.5f);
    strip.setAdvanceVisibility(isAdv);
}

void MultibandCompressor5BAudioProcessorEditor::updateOversamplingButtons(int activeIndex)
{
    juce::Colour activeCol(0xff4cd137);
    juce::Colour inactiveCol(0xff2d3436);

    for (int i = 0; i < 5; ++i)
    {
        bool isActive = (i == activeIndex);
        osButtons[i].setColour(juce::TextButton::buttonColourId, isActive ? activeCol.withAlpha(0.25f) : inactiveCol);
        osButtons[i].setColour(juce::TextButton::textColourOnId, isActive ? activeCol : juce::Colour(0xffb5c0d0));
        osButtons[i].setColour(juce::TextButton::textColourOffId, isActive ? activeCol : juce::Colour(0xff8c96a5));
    }
}

void MultibandCompressor5BAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181a1f));

    // Outer metal chassis border
    g.setColour(juce::Colour(0xff2d333f));
    g.drawRect(getLocalBounds(), 2);

    // Header Title
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffe1e8f0));
    g.drawText("5 Multiband Compressor", 20, 14, 250, 36, juce::Justification::centredLeft);

    // Vertical console divider between 5 Bands and Master Strip
    float divX = static_cast<float>(getWidth() - 14 - 160 - 5);
    g.setColour(juce::Colour(0xff2d333f));
    g.drawVerticalLine(static_cast<int>(divX), 64.0f, static_cast<float>(getHeight() - 56));
    g.setColour(juce::Colour(0xff101215));
    g.drawVerticalLine(static_cast<int>(divX) + 1, 64.0f, static_cast<float>(getHeight() - 56));
}

void MultibandCompressor5BAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(14);

    // 1. Header (Gear button, Mode switch button, Reset to default, and Preset Manager)
    auto headerArea = area.removeFromTop(44);
    gearButton.setBounds(headerArea.removeFromRight(34).reduced(0, 5));
    headerArea.removeFromRight(10);
    modeToggleButton.setBounds(headerArea.removeFromRight(135).reduced(0, 4));
    headerArea.removeFromRight(8);
    resetDefaultButton.setBounds(headerArea.removeFromRight(105).reduced(0, 6));
    headerArea.removeFromRight(14);

    loadPresetButton.setBounds(headerArea.removeFromRight(50).reduced(0, 6));
    headerArea.removeFromRight(6);
    savePresetButton.setBounds(headerArea.removeFromRight(50).reduced(0, 6));
    headerArea.removeFromRight(8);
    presetComboBox.setBounds(headerArea.removeFromRight(220).reduced(0, 6));
    headerArea.removeFromRight(4);
    presetLabel.setBounds(headerArea.removeFromRight(55).reduced(0, 6));

    if (settingsPanel != nullptr)
    {
        int pWidth = 360;
        int pHeight = 150;
        int pX = juce::jlimit(14, getWidth() - pWidth - 14, gearButton.getRight() - pWidth);
        int pY = gearButton.getBottom() + 4;
        settingsPanel->setBounds(pX, pY, pWidth, pHeight);
    }

    // 2. Bottom Footer (Oversampling Bar & Latency Status)
    auto footerArea = area.removeFromBottom(36);
    oversamplingTitleLabel.setBounds(footerArea.removeFromLeft(110));
    for (int i = 0; i < 5; ++i)
    {
        osButtons[i].setBounds(footerArea.removeFromLeft(64).reduced(2, 4));
    }
    latencyStatusLabel.setBounds(footerArea.removeFromRight(220));

    // 3. Middle Area: 5 Bands (Left) + Dedicated Master Column (Right)
    area.removeFromTop(6);
    area.removeFromBottom(6);

    const int masterWidth = 160;
    auto masterArea = area.removeFromRight(masterWidth);
    area.removeFromRight(10); // Clear gap between 5 bands and master strip

    // Aligned Top Row: Crossover Display flush with 5 bands, Master Meter flush with Master strip
    crossoverDisplay.setBounds(area.removeFromTop(160));
    if (masterMeterPanel != nullptr)
        masterMeterPanel->setBounds(masterArea.removeFromTop(160));

    // Vertical spacing between top graphs and lower controls
    area.removeFromTop(8);
    masterArea.removeFromTop(8);

    // 4. Five Band Channel Strips
    const int totalBands = 5;
    const int bandSpacing = 6;
    const int availableBandWidth = area.getWidth() - (bandSpacing * (totalBands - 1));
    const int bandWidth = availableBandWidth / totalBands;

    BandStripControls* bands[5] = { &subBand, &lowBand, &midBand, &midHighBand, &highBand };

    const bool isAdv = modeToggleButton.getToggleState();

    for (int i = 0; i < 5; ++i)
    {
        auto stripArea = (i == 4) ? area : area.removeFromLeft(bandWidth);
        if (i < 4) area.removeFromLeft(bandSpacing);

        auto* strip = bands[i];
        strip->setAdvanceVisibility(isAdv);

        auto col = stripArea;

        // Title & Frequency Range
        strip->titleLabel.setBounds(col.removeFromTop(20));
        strip->freqRangeLabel.setBounds(col.removeFromTop(16));

        // Compression Transfer Curve Graph (120px)
        col.removeFromTop(4);
        strip->transferGraph->setBounds(col.removeFromTop(120).reduced(2, 0));

        // Mute / Solo / Bypass buttons row
        col.removeFromTop(6);
        auto btnRow = col.removeFromTop(22).reduced(4, 0);
        int bw = btnRow.getWidth() / 3;
        strip->soloButton.setBounds(btnRow.removeFromLeft(bw).reduced(1, 0));
        strip->muteButton.setBounds(btnRow.removeFromLeft(bw).reduced(1, 0));
        strip->bypassButton.setBounds(btnRow.reduced(1, 0));

        if (!isAdv)
        {
            // NORMAL MODE - Authentic 1:1 Analog Sized Knobs (~100px diameter)
            col.removeFromTop(6);
            strip->modeButton.setBounds(col.removeFromTop(22).reduced(12, 0));

            col.removeFromTop(8);
            auto prArea = col.removeFromTop(130);
            strip->peakReductionLabel.setBounds(prArea.removeFromTop(15));
            strip->peakReductionSlider.setBounds(prArea);

            col.removeFromTop(8);
            auto gainArea = col.removeFromTop(130);
            strip->gainLabel.setBounds(gainArea.removeFromTop(15));
            strip->gainSlider.setBounds(gainArea);

            col.removeFromTop(8);
            auto warmthArea = col.removeFromTop(130);
            strip->warmthLabel.setBounds(warmthArea.removeFromTop(15));
            strip->warmthSlider.setBounds(warmthArea);
        }
        else
        {
            // ADVANCE MODE - Expanded Dual Knob Pairs (~80px diameter)
            col.removeFromTop(4);
            strip->engineButton.setBounds(col.removeFromTop(20).reduced(8, 0));

            auto layoutKnobPair = [&col](juce::Slider& s1, juce::Label& l1, juce::Slider& s2, juce::Label& l2)
            {
                auto pairArea = col.removeFromTop(100);
                auto leftHalf = pairArea.removeFromLeft(pairArea.getWidth() / 2);
                auto rightHalf = pairArea;

                l1.setBounds(leftHalf.removeFromTop(14));
                s1.setBounds(leftHalf);

                l2.setBounds(rightHalf.removeFromTop(14));
                s2.setBounds(rightHalf);
            };

            col.removeFromTop(4);
            layoutKnobPair(strip->thresholdSlider, strip->thresholdLabel, strip->ratioSlider, strip->ratioLabel);

            col.removeFromTop(4);
            layoutKnobPair(strip->attackSlider, strip->attackLabel, strip->releaseSlider, strip->releaseLabel);

            col.removeFromTop(4);
            layoutKnobPair(strip->kneeSlider, strip->kneeLabel, strip->bandMixSlider, strip->bandMixLabel);

            col.removeFromTop(4);
            layoutKnobPair(strip->gainSlider, strip->gainLabel, strip->warmthSlider, strip->warmthLabel);
        }
    }

    // 5. Layout Master Channel Strip (Underneath Master Meter)
    auto mCol = masterArea;
    masterTitleLabel.setBounds(mCol.removeFromTop(20));
    masterSubLabel.setBounds(mCol.removeFromTop(16));

    mCol.removeFromTop(10);
    auto inGainArea = mCol.removeFromTop(134).reduced(6, 0);
    inputGainLabel.setBounds(inGainArea.removeFromTop(16));
    inputGainSlider.setBounds(inGainArea);

    mCol.removeFromTop(8);
    auto outGainArea = mCol.removeFromTop(134).reduced(6, 0);
    outputGainLabel.setBounds(outGainArea.removeFromTop(16));
    outputGainSlider.setBounds(outGainArea);

    mCol.removeFromTop(8);
    auto mixArea = mCol.removeFromTop(134).reduced(6, 0);
    mixLabel.setBounds(mixArea.removeFromTop(16));
    mixSlider.setBounds(mixArea);
}

void MultibandCompressor5BAudioProcessorEditor::timerCallback()
{
    if (!isShowing())
        return;

    // Update live master meters
    if (masterMeterPanel != nullptr)
        masterMeterPanel->setLevels(audioProcessor.getInputLevel(), audioProcessor.getOutputLevel());

    // Update crossover display
    crossoverDisplay.setGainReductions(
        audioProcessor.getSubGainReduction(),
        audioProcessor.getLowGainReduction(),
        audioProcessor.getMidGainReduction(),
        audioProcessor.getMidHighGainReduction(),
        audioProcessor.getHighGainReduction()
    );

    // Update dynamic frequency range labels (cached to avoid redundant string allocations at 30Hz)
    float f1 = audioProcessor.getFreq1();
    float f2 = audioProcessor.getFreq2();
    float f3 = audioProcessor.getFreq3();
    float f4 = audioProcessor.getFreq4();

    if (f1 != lastFreq1 || f2 != lastFreq2 || f3 != lastFreq3 || f4 != lastFreq4)
    {
        lastFreq1 = f1;
        lastFreq2 = f2;
        lastFreq3 = f3;
        lastFreq4 = f4;

        subBand.freqRangeLabel.setText("20Hz - " + juce::String(static_cast<int>(f1)) + "Hz", juce::dontSendNotification);
        lowBand.freqRangeLabel.setText(juce::String(static_cast<int>(f1)) + "Hz - " + juce::String(static_cast<int>(f2)) + "Hz", juce::dontSendNotification);
        midBand.freqRangeLabel.setText(juce::String(static_cast<int>(f2)) + "Hz - " + juce::String(f3 / 1000.0f, 2) + "kHz", juce::dontSendNotification);
        midHighBand.freqRangeLabel.setText(juce::String(f3 / 1000.0f, 2) + "k - " + juce::String(f4 / 1000.0f, 1) + "kHz", juce::dontSendNotification);
        highBand.freqRangeLabel.setText(juce::String(f4 / 1000.0f, 1) + "kHz - 20kHz", juce::dontSendNotification);
    }

    // Update transfer graphs
    const bool isAdv = modeToggleButton.getToggleState();

    auto updateGraph = [this, isAdv](BandStripControls& strip, const juce::String& prefix, float currentGr)
    {
        if (strip.transferGraph)
        {
            float th, rat, knee;
            if (!isAdv)
            {
                float pr = audioProcessor.getAPVTS().getRawParameterValue(prefix + "_peak_reduction")->load();
                bool lim = audioProcessor.getAPVTS().getRawParameterValue(prefix + "_mode")->load() > 0.5f;
                th = 6.0f - (pr * 0.52f);
                rat = lim ? 10.0f : 3.2f;
                knee = lim ? 8.0f : 16.0f;
            }
            else
            {
                th = audioProcessor.getAPVTS().getRawParameterValue(prefix + "_threshold")->load();
                rat = audioProcessor.getAPVTS().getRawParameterValue(prefix + "_ratio")->load();
                knee = audioProcessor.getAPVTS().getRawParameterValue(prefix + "_knee")->load();
            }
            strip.transferGraph->setParameters(th, rat, knee, currentGr);
        }
    };

    updateGraph(subBand,     "sub",      audioProcessor.getSubGainReduction());
    updateGraph(lowBand,     "low",      audioProcessor.getLowGainReduction());
    updateGraph(midBand,     "mid",      audioProcessor.getMidGainReduction());
    updateGraph(midHighBand, "mid_high", audioProcessor.getMidHighGainReduction());
    updateGraph(highBand,    "high",     audioProcessor.getHighGainReduction());

    // Update oversampling bar
    int osIdx = audioProcessor.getCurrentOversamplingIndex();
    updateOversamplingButtons(osIdx);

    int latency = audioProcessor.getCurrentLatencySamples();
    if (latency > 0)
    {
        double ms = (latency / audioProcessor.getSampleRate()) * 1000.0;
        latencyStatusLabel.setText("Latency: " + juce::String(latency) + " smp (" + juce::String(ms, 1) + " ms)",
                                   juce::dontSendNotification);
    }
    else
    {
        latencyStatusLabel.setText("Zero Latency (Realtime)", juce::dontSendNotification);
    }

    if (isAdv != lastKnownAdvanceMode)
    {
        lastKnownAdvanceMode = isAdv;
        resized();
    }
}

void MultibandCompressor5BAudioProcessorEditor::updatePresetList(const juce::String& presetToSelect)
{
    presetComboBox.clear(juce::dontSendNotification);
    auto presets = audioProcessor.getPresetManager().getAvailablePresets();

    int idToSelect = -1;
    for (int i = 0; i < presets.size(); ++i)
    {
        int itemId = i + 1;
        presetComboBox.addItem(presets[i], itemId);
        if (presetToSelect.isNotEmpty() && presets[i].equalsIgnoreCase(presetToSelect))
            idToSelect = itemId;
    }

    if (idToSelect > 0)
        presetComboBox.setSelectedId(idToSelect, juce::dontSendNotification);
}

void MultibandCompressor5BAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (settingsPanel != nullptr && settingsPanel->isVisible())
    {
        auto pos = e.getEventRelativeTo(this).getPosition();
        if (!settingsPanel->getBounds().contains(pos) && !gearButton.getBounds().contains(pos))
        {
            settingsPanel->setVisible(false);
            gearButton.setToggleState(false, juce::dontSendNotification);
        }
    }
}
