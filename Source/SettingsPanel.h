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
        setClickingTogglesState(true);
        setTooltip("Open plugin settings & engine info");
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        bool isActive = getToggleState();

        // 1. Chassis Background
        juce::Colour bgCol = isActive ? juce::Colour(0xff00d2d3).withAlpha(0.2f)
                                      : (shouldDrawButtonAsHighlighted ? juce::Colour(0xff2d3542) : juce::Colour(0xff1e222a));
        g.setColour(bgCol);
        g.fillRoundedRectangle(bounds, 4.0f);

        // 2. Bezel Border
        juce::Colour borderCol = isActive ? juce::Colour(0xff00d2d3)
                                          : (shouldDrawButtonAsHighlighted ? juce::Colour(0xff57606f) : juce::Colour(0xff2f3542));
        g.setColour(borderCol);
        g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

        // 3. Draw Cog Icon via Path
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float outerR = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.34f;
        float innerR = outerR * 0.65f;
        float holeR  = outerR * 0.32f;
        const int numTeeth = 6;

        juce::Path gear;
        for (int i = 0; i < numTeeth * 2; ++i)
        {
            float angle = i * juce::MathConstants<float>::twoPi / (numTeeth * 2);
            float r = (i % 2 == 0) ? outerR : innerR;
            float px = cx + r * std::cos(angle);
            float py = cy + r * std::sin(angle);

            if (i == 0)
                gear.startNewSubPath(px, py);
            else
                gear.lineTo(px, py);
        }
        gear.closeSubPath();
        gear.addEllipse(cx - holeR, cy - holeR, holeR * 2.0f, holeR * 2.0f);
        gear.setUsingNonZeroWinding(false); // Punch central hole

        juce::Colour gearCol = isActive ? juce::Colour(0xff00d2d3)
                                        : (shouldDrawButtonAsHighlighted ? juce::Colour(0xfff0f4f8) : juce::Colour(0xffa4b0be));
        if (shouldDrawButtonAsDown)
            gearCol = gearCol.darker(0.2f);

        g.setColour(gearCol);
        g.fillPath(gear);
    }
};

//==============================================================================
/** Settings & Engine Info Panel popover (Pure CPU Software Rendering). */
class SettingsPanel : public juce::Component
{
public:
    std::function<void()> onClose;

    SettingsPanel(juce::AudioProcessorValueTreeState& apvts)
    {
        juce::ignoreUnused(apvts);

        // 1. Title Bar
        titleLabel.setText("SETTINGS & ENGINE INFO", juce::dontSendNotification);
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

        // 2. Section Header: Rendering Engine
        sectionLabel.setText("GRAPHICS & STABILITY ENGINE", juce::dontSendNotification);
        sectionLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        sectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00d2d3));
        addAndMakeVisible(sectionLabel);

        // 3. Engine Status (Pure CPU)
        engineLabel.setText("Rendering Mode: Pure CPU (DirectWrite / GDI)", juce::dontSendNotification);
        engineLabel.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        engineLabel.setColour(juce::Label::textColourId, juce::Colour(0xff2ed573)); // Fresh green
        addAndMakeVisible(engineLabel);

        engineDescLabel.setText("Hardware acceleration (GPU/OpenGL) has been completely removed to guarantee 100% crash-free stability across Equalizer APO, OBS Studio, and all DAWs.", juce::dontSendNotification);
        engineDescLabel.setFont(juce::FontOptions(9.5f));
        engineDescLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
        addAndMakeVisible(engineDescLabel);
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

        engineLabel.setBounds(area.removeFromTop(22));
        area.removeFromTop(2);
        engineDescLabel.setBounds(area.removeFromTop(36));
    }

private:
    juce::Label titleLabel;
    juce::TextButton closeButton;
    juce::Label sectionLabel;
    juce::Label engineLabel;
    juce::Label engineDescLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};
