#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/** Gear Button with vector-drawn cog icon and Yamaha hardware aesthetics. */
class GearButton : public juce::Button
{
public:
    GearButton() : juce::Button("Settings")
    {
        setTooltip("Settings");
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        bool isActive = getToggleState();

        // Background
        juce::Colour bg = isActive ? juce::Colour(0xff222834)
                                   : (shouldDrawButtonAsHighlighted ? juce::Colour(0xff2b313d) : juce::Colour(0xff1d2129));
        if (shouldDrawButtonAsDown)
            bg = bg.darker(0.12f);

        g.setColour(bg);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border outline
        juce::Colour borderCol = isActive ? juce::Colour(0xff00d2d3)
                                          : (shouldDrawButtonAsHighlighted ? juce::Colour(0xff4f5b6e) : juce::Colour(0xff333a46));
        g.setColour(borderCol);
        g.drawRoundedRectangle(bounds, 4.0f, isActive ? 1.5f : 1.0f);

        // Vector Gear Icon with 6 trapezoidal teeth and central bore
        auto c = bounds.getCentre();
        float outerR = bounds.getWidth() * 0.32f;
        float innerR = outerR * 0.72f;
        float holeR  = outerR * 0.36f;
        const int numTeeth = 6;

        juce::Path gear;
        const float toothAngle = juce::MathConstants<float>::twoPi / static_cast<float>(numTeeth);
        const float halfTooth  = toothAngle * 0.25f;

        for (int i = 0; i < numTeeth; ++i)
        {
            float midAngle = static_cast<float>(i) * toothAngle;
            float a1 = midAngle - halfTooth * 1.5f;
            float a2 = midAngle - halfTooth * 0.7f;
            float a3 = midAngle + halfTooth * 0.7f;
            float a4 = midAngle + halfTooth * 1.5f;

            float x1 = c.x + innerR * std::cos(a1);
            float y1 = c.y + innerR * std::sin(a1);
            float x2 = c.x + outerR * std::cos(a2);
            float y2 = c.y + outerR * std::sin(a2);
            float x3 = c.x + outerR * std::cos(a3);
            float y3 = c.y + outerR * std::sin(a3);
            float x4 = c.x + innerR * std::cos(a4);
            float y4 = c.y + innerR * std::sin(a4);

            if (i == 0)
                gear.startNewSubPath(x1, y1);
            else
                gear.lineTo(x1, y1);

            gear.lineTo(x2, y2);
            gear.lineTo(x3, y3);
            gear.lineTo(x4, y4);
        }
        gear.closeSubPath();
        gear.addEllipse(c.x - holeR, c.y - holeR, holeR * 2.0f, holeR * 2.0f);
        gear.setUsingNonZeroWinding(false);

        juce::Colour gearCol = isActive ? juce::Colour(0xff00d2d3)
                                        : (shouldDrawButtonAsHighlighted ? juce::Colour(0xffe1e8f0) : juce::Colour(0xff94a3b8));
        g.setColour(gearCol);
        g.fillPath(gear);
    }
};

//==============================================================================
/** Extensible Settings Panel popover containing GPU acceleration and future options. */
class SettingsPanel : public juce::Component
{
public:
    std::function<void()> onClose;
    std::function<void(bool)> onGpuAccelerationChanged;

    SettingsPanel(juce::AudioProcessorValueTreeState& apvts)
    {
        // 1. Title Bar
        titleLabel.setText("SETTINGS", juce::dontSendNotification);
        titleLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe1e8f0));
        addAndMakeVisible(titleLabel);

        closeButton.setButtonText(juce::String::charToString(0x00D7)); // "×"
        closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222731));
        closeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb5c0d0));
        closeButton.onClick = [this]
        {
            if (onClose)
                onClose();
            else
                setVisible(false);
        };
        addAndMakeVisible(closeButton);

        // 2. Section Header: Hardware & Performance
        sectionLabel.setText("HARDWARE & PERFORMANCE", juce::dontSendNotification);
        sectionLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        sectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00d2d3));
        addAndMakeVisible(sectionLabel);

        // 3. Row: GPU Acceleration (OpenGL)
        gpuLabel.setText("GPU Acceleration (OpenGL)", juce::dontSendNotification);
        gpuLabel.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        gpuLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff0f4f8));
        addAndMakeVisible(gpuLabel);

        gpuDescLabel.setText("Hardware-accelerates GUI rendering via OpenGL. Default is OFF for stability on systems without dedicated GPU.", juce::dontSendNotification);
        gpuDescLabel.setFont(juce::FontOptions(9.5f));
        gpuDescLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
        addAndMakeVisible(gpuDescLabel);

        gpuToggleButton.setButtonText("OFF");
        gpuToggleButton.getProperties().set("btnColour", static_cast<int>(juce::Colour(0xff00d2d3).getARGB()));
        gpuToggleButton.onClick = [this]
        {
            bool isGpu = gpuToggleButton.getToggleState();
            gpuToggleButton.setButtonText(isGpu ? "ON" : "OFF");
            if (onGpuAccelerationChanged)
                onGpuAccelerationChanged(isGpu);
        };
        addAndMakeVisible(gpuToggleButton);

        gpuAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "gpu_accel", gpuToggleButton);

        bool initGpu = apvts.getRawParameterValue("gpu_accel")->load() > 0.5f;
        gpuToggleButton.setToggleState(initGpu, juce::dontSendNotification);
        gpuToggleButton.setButtonText(initGpu ? "ON" : "OFF");
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Dark backplate with drop shadow feel
        g.setColour(juce::Colour(0xf6161920));
        g.fillRoundedRectangle(bounds, 6.0f);

        // Outer chassis border
        g.setColour(juce::Colour(0xff2d3542));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.5f);

        // Accent top edge line
        g.setColour(juce::Colour(0xff00d2d3).withAlpha(0.6f));
        g.drawLine(bounds.getX() + 6.0f, bounds.getY() + 1.0f, bounds.getRight() - 6.0f, bounds.getY() + 1.0f, 1.5f);

        // Header separator
        g.setColour(juce::Colour(0xff222731));
        g.fillRect(0.0f, 32.0f, bounds.getWidth(), 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);

        // Title row
        auto titleArea = area.removeFromTop(20);
        closeButton.setBounds(titleArea.removeFromRight(22).reduced(1));
        titleLabel.setBounds(titleArea);

        area.removeFromTop(10);
        sectionLabel.setBounds(area.removeFromTop(16));
        area.removeFromTop(6);

        // GPU Row
        auto gpuRow = area.removeFromTop(26);
        gpuToggleButton.setBounds(gpuRow.removeFromRight(64).reduced(0, 2));
        gpuRow.removeFromRight(10);
        gpuLabel.setBounds(gpuRow);

        area.removeFromTop(2);
        gpuDescLabel.setBounds(area.removeFromTop(30));

        // Future setting rows can simply be added below without changing overall architecture
    }

private:
    juce::Label titleLabel;
    juce::TextButton closeButton;
    juce::Label sectionLabel;
    juce::Label gpuLabel;
    juce::Label gpuDescLabel;
    juce::ToggleButton gpuToggleButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gpuAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};
