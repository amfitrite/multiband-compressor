#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class YamahaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    YamahaLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff181a1f));
        setColour(juce::Label::textColourId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        if (width < 12 || height < 12)
            return;

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        if (radius < 6.0f)
            return;

        auto center = bounds.getCentre();

        // Band accent colour stored in dynamic properties if available, else default to cyan
        juce::Colour accentCol = juce::Colour(0xff00d2d3);
        if (slider.getProperties().contains("accentColour"))
            accentCol = juce::Colour(static_cast<juce::uint32>(static_cast<int>(slider.getProperties()["accentColour"])));

        const bool isBigMainKnob = (radius >= 30.0f);

        // --- 0. CALIBRATED ANALOG HASH MARKS AROUND DIAL ---
        const int numTicks = isBigMainKnob ? 13 : 9;
        for (int i = 0; i < numTicks; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(numTicks - 1);
            float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            float cosA = std::cos(a - juce::MathConstants<float>::halfPi);
            float sinA = std::sin(a - juce::MathConstants<float>::halfPi);

            float r1 = radius - 0.5f;
            float r2 = radius - (isBigMainKnob ? 4.0f : 2.5f);
            bool isMajor = (i == 0 || i == numTicks - 1 || i == (numTicks - 1) / 2);

            g.setColour(isMajor ? juce::Colour(0xff94a3b8) : juce::Colour(0xff333d4b));
            g.drawLine(center.x + r2 * cosA, center.y + r2 * sinA,
                       center.x + r1 * cosA, center.y + r1 * sinA, isMajor ? 1.6f : 1.0f);
        }

        // --- 1. DRAW LED HALO TRACK (GROOVE) ---
        const float trackThickness = isBigMainKnob ? 3.5f : 2.5f;
        const float trackRadius = radius - (isBigMainKnob ? 5.5f : 3.8f);

        juce::Path trackBg;
        trackBg.addCentredArc(center.x, center.y, trackRadius, trackRadius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff0d0f13));
        g.strokePath(trackBg, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active LED arc
        auto currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        juce::Path trackVal;
        trackVal.addCentredArc(center.x, center.y, trackRadius, trackRadius, 0.0f,
                               rotaryStartAngle, currentAngle, true);
        
        // Arc glow & solid
        g.setColour(accentCol.withAlpha(0.35f));
        g.strokePath(trackVal, juce::PathStrokeType(trackThickness + 2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(accentCol);
        g.strokePath(trackVal, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // --- 2. HEAVY HARDWARE CHASSIS DROP SHADOW & SKIRT ---
        auto knobRadius = trackRadius - (isBigMainKnob ? 4.5f : 3.0f);
        if (knobRadius < 8.0f) knobRadius = 8.0f;
        auto knobBounds = juce::Rectangle<float>(center.x - knobRadius, center.y - knobRadius,
                                                 knobRadius * 2.0f, knobRadius * 2.0f);

        // Outer chassis bevel skirt
        g.setColour(juce::Colour(0xff14171d));
        g.fillEllipse(knobBounds.expanded(1.5f));

        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillEllipse(knobBounds.translated(0.0f, 2.5f));

        // --- 3. MACHINED OUTER FLUTED / KNURLED RING (ANALOG GRIP) ---
        juce::ColourGradient rimGrad(juce::Colour(0xff4a5464), center.x - knobRadius * 0.4f, center.y - knobRadius,
                                     juce::Colour(0xff14161c), center.x + knobRadius * 0.4f, center.y + knobRadius, false);
        g.setGradientFill(rimGrad);
        g.fillEllipse(knobBounds);

        // Tactile knurling notches around the circumference with highlight and shadow
        const int numNotches = isBigMainKnob ? 28 : 20;
        for (int i = 0; i < numNotches; ++i)
        {
            float angle = (static_cast<float>(i) / static_cast<float>(numNotches)) * juce::MathConstants<float>::twoPi;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            float rOuter = knobRadius - 0.5f;
            float rInner = knobRadius - (isBigMainKnob ? 4.0f : 2.8f);

            // Shadow notch edge
            g.setColour(juce::Colour(0xff0d0f13).withAlpha(0.85f));
            g.drawLine(center.x + rInner * cosA, center.y + rInner * sinA,
                       center.x + rOuter * cosA, center.y + rOuter * sinA, 1.4f);

            // Highlight notch edge
            g.setColour(juce::Colour(0xff68778d).withAlpha(0.45f));
            g.drawLine(center.x + rInner * cosA - 0.7f, center.y + rInner * sinA - 0.7f,
                       center.x + rOuter * cosA - 0.7f, center.y + rOuter * sinA - 0.7f, 0.9f);
        }

        // --- 4. BEVELED INNER METAL / BAKELITE CAP ---
        auto innerBounds = knobBounds.reduced(isBigMainKnob ? 4.5f : 3.2f);
        bool isWarmthKnob = slider.getProperties().contains("isWarmthKnob");

        if (isWarmthKnob)
        {
            // Warm, tube-like glowing inner cap for the Warmth slider
            juce::ColourGradient capGrad(juce::Colour(0xff4a2311), center.x, center.y,
                                         juce::Colour(0xff1f0c05), center.x, center.y + innerBounds.getHeight() * 0.5f, true);
            g.setGradientFill(capGrad);
        }
        else
        {
            juce::ColourGradient capGrad(juce::Colour(0xff333a46), center.x - innerBounds.getWidth() * 0.35f, center.y - innerBounds.getHeight() * 0.35f,
                                         juce::Colour(0xff13161a), center.x + innerBounds.getWidth() * 0.35f, center.y + innerBounds.getHeight() * 0.35f, false);
            g.setGradientFill(capGrad);
        }

        g.fillEllipse(innerBounds);

        // Machined concentric ring highlights (sunburst / lathe lines)
        g.setColour(isWarmthKnob ? juce::Colour(0xff8c3b1a).withAlpha(0.6f) : juce::Colour(0xff576579).withAlpha(0.6f));
        g.drawEllipse(innerBounds, 1.3f);

        auto coreBounds = innerBounds.reduced(isBigMainKnob ? 5.5f : 3.5f);

        if (isWarmthKnob)
        {
            // Simulating a glowing tube filament core
            g.setColour(juce::Colour(0xffe64a19));
            g.fillEllipse(coreBounds);
            g.setColour(juce::Colour(0xffff8a65).withAlpha(0.8f));
            g.drawEllipse(coreBounds, 1.5f);

            // Soft inner glow
            juce::ColourGradient glowGrad(juce::Colour(0xffffa726).withAlpha(0.4f), center.x, center.y,
                                          juce::Colours::transparentWhite, center.x, center.y + coreBounds.getHeight() * 0.5f, true);
            g.setGradientFill(glowGrad);
            g.fillEllipse(coreBounds);
        }
        else
        {
            g.setColour(juce::Colour(0xff222832));
            g.fillEllipse(coreBounds);
            g.setColour(juce::Colour(0xff3f4b5c).withAlpha(0.45f));
            g.drawEllipse(coreBounds, 1.0f);
        }

        // --- 5. PRECISION ENGRAVED ANALOG POINTER NEEDLE ---
        float needleStartR = isBigMainKnob ? 4.5f : 2.5f;
        float needleEndR = knobRadius - 2.5f;

        float needleAngle = currentAngle - juce::MathConstants<float>::halfPi;
        float cosN = std::cos(needleAngle);
        float sinN = std::sin(needleAngle);

        juce::Point<float> pStart(center.x + needleStartR * cosN, center.y + needleStartR * sinN);
        juce::Point<float> pEnd(center.x + needleEndR * cosN, center.y + needleEndR * sinN);

        // Needle Shadow
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(pStart.x + 0.8f, pStart.y + 0.8f, pEnd.x + 0.8f, pEnd.y + 0.8f, isBigMainKnob ? 2.6f : 2.0f);

        // Needle Line (White recessed enamel or warm glowing orange line)
        g.setColour(isWarmthKnob ? juce::Colour(0xffffb74d) : juce::Colours::white);
        g.drawLine(pStart.x, pStart.y, pEnd.x, pEnd.y, isBigMainKnob ? 2.2f : 1.7f);

        // Pointer Tip Glowing Dot
        g.setColour(isWarmthKnob ? juce::Colour(0xffff8a65).withAlpha(0.9f) : accentCol.withAlpha(0.85f));
        g.fillEllipse(pEnd.x - 2.8f, pEnd.y - 2.8f, 5.6f, 5.6f);

        g.setColour(isWarmthKnob ? juce::Colour(0xffffe0b2) : juce::Colours::white);
        g.fillEllipse(pEnd.x - 1.3f, pEnd.y - 1.3f, 2.6f, 2.6f);
    }


    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        if (button.getWidth() < 10 || button.getHeight() < 10)
            return;

        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        bool isOn = button.getToggleState();

        juce::Colour glowColour = juce::Colour(0xff00d2d3);
        if (button.getProperties().contains("btnColour"))
            glowColour = juce::Colour(static_cast<juce::uint32>(static_cast<int>(button.getProperties()["btnColour"])));

        // Background
        juce::Colour bg = isOn ? glowColour.withMultipliedBrightness(0.35f) : juce::Colour(0xff20242b);
        if (shouldDrawButtonAsHighlighted)
            bg = bg.brighter(0.12f);
        if (shouldDrawButtonAsDown)
            bg = bg.darker(0.1f);

        g.setColour(bg);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border outline
        juce::Colour border = isOn ? glowColour : juce::Colour(0xff3b414d);
        g.setColour(border);
        g.drawRoundedRectangle(bounds, 4.0f, isOn ? 1.8f : 1.0f);

        // Text
        float fontSize = (button.getButtonText().length() > 5) ? 9.0f : 10.0f;
        g.setFont(juce::FontOptions(fontSize, juce::Font::bold));
        g.setColour(isOn ? (glowColour.getBrightness() > 0.6f ? juce::Colours::white : glowColour.brighter()) : juce::Colour(0xff8c96a5));
        g.drawFittedText(button.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, 1, 0.75f);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll(label.findColour(juce::Label::backgroundColourId));

        if (!label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const juce::Font font(label.getFont());

            g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
            g.setFont(font);

            auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

            g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                             juce::jmax(1, static_cast<int>(static_cast<float>(textArea.getHeight()) / font.getHeight())),
                             label.getMinimumHorizontalScale());
        }
    }
};
