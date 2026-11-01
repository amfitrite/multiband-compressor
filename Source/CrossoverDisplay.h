#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

class CrossoverDisplay : public juce::Component
{
public:
    CrossoverDisplay(juce::AudioProcessorValueTreeState& state)
        : apvts(state)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void setGainReductions(float gr1, float gr2, float gr3, float gr4, float gr5)
    {
        if (std::abs(grDb1 - gr1) < 0.1f &&
            std::abs(grDb2 - gr2) < 0.1f &&
            std::abs(grDb3 - gr3) < 0.1f &&
            std::abs(grDb4 - gr4) < 0.1f &&
            std::abs(grDb5 - gr5) < 0.1f)
            return;

        grDb1 = gr1;
        grDb2 = gr2;
        grDb3 = gr3;
        grDb4 = gr4;
        grDb5 = gr5;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() < 80.0f || bounds.getHeight() < 50.0f)
            return;

        // Dark brushed display background
        juce::ColourGradient bgGrad(juce::Colour(0xff121417), 0.0f, 0.0f,
                                    juce::Colour(0xff1a1d22), 0.0f, bounds.getHeight(), false);
        g.setGradientFill(bgGrad);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Grid lines (Frequency)
        g.setColour(juce::Colour(0xff2a303a));
        float fGrid[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
        for (float f : fGrid)
        {
            float x = freqToX(f, bounds.getWidth());
            g.drawVerticalLine(static_cast<int>(x), bounds.getY() + 4.0f, bounds.getBottom() - 16.0f);
        }

        // Horizontal dB lines: 0dB, -6dB, -12dB, -18dB, -24dB
        float zeroY = dbToY(0.0f, bounds.getHeight());
        g.setColour(juce::Colour(0xff3f4857));
        g.drawHorizontalLine(static_cast<int>(zeroY), bounds.getX() + 4.0f, bounds.getRight() - 4.0f);

        g.setColour(juce::Colour(0xff232832));
        float dbMarks[] = { -6.0f, -12.0f, -18.0f, -24.0f };
        for (float db : dbMarks)
        {
            float y = dbToY(db, bounds.getHeight());
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX() + 4.0f, bounds.getRight() - 4.0f);
        }

        // Safe parameter reading
        float f1 = getParamSafe("sub_low_freq", 90.0f);
        float f2 = getParamSafe("low_mid_freq", 500.0f);
        float f3 = getParamSafe("mid_high_freq", 1200.0f);
        float f4 = getParamSafe("high_super_freq", 5000.0f);

        float x1 = freqToX(f1, bounds.getWidth());
        float x2 = freqToX(f2, bounds.getWidth());
        float x3 = freqToX(f3, bounds.getWidth());
        float x4 = freqToX(f4, bounds.getWidth());

        // 5 Band region fills with live gain reduction curves
        drawBandRegion(g, bounds, 0.0f, x1, grDb1, juce::Colour(0xffe67e22), "SUB (20-90Hz)");
        drawBandRegion(g, bounds, x1, x2, grDb2, juce::Colour(0xfff1c40f), "LOW (90-500Hz)");
        drawBandRegion(g, bounds, x2, x3, grDb3, juce::Colour(0xff00d2d3), "MID (500-1.2k)");
        drawBandRegion(g, bounds, x3, x4, grDb4, juce::Colour(0xff3b82f6), "MID-HIGH (1.2k-5k)");
        drawBandRegion(g, bounds, x4, bounds.getWidth(), grDb5, juce::Colour(0xffb05bf5), "AIR (5k-20k)");

        // 4 Draggable Splitters
        drawSplitter(g, bounds, x1, f1, hoveredSplitter == 1 || draggingSplitter == 1, "SUB / LOW");
        drawSplitter(g, bounds, x2, f2, hoveredSplitter == 2 || draggingSplitter == 2, "LOW / MID");
        drawSplitter(g, bounds, x3, f3, hoveredSplitter == 3 || draggingSplitter == 3, "MID / HIGH");
        drawSplitter(g, bounds, x4, f4, hoveredSplitter == 4 || draggingSplitter == 4, "HIGH / AIR");

        // Outer beveled frame
        g.setColour(juce::Colour(0xff353b47));
        g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

        // Frequency axis labels at bottom
        g.setFont(juce::FontOptions(10.0f));
        g.setColour(juce::Colour(0xff707b8c));
        g.drawText("100Hz", static_cast<int>(freqToX(100.0f, bounds.getWidth()) - 20), static_cast<int>(bounds.getBottom() - 15), 40, 12, juce::Justification::centred);
        g.drawText("500Hz", static_cast<int>(freqToX(500.0f, bounds.getWidth()) - 20), static_cast<int>(bounds.getBottom() - 15), 40, 12, juce::Justification::centred);
        g.drawText("1.2kHz", static_cast<int>(freqToX(1200.0f, bounds.getWidth()) - 20), static_cast<int>(bounds.getBottom() - 15), 40, 12, juce::Justification::centred);
        g.drawText("5kHz", static_cast<int>(freqToX(5000.0f, bounds.getWidth()) - 20), static_cast<int>(bounds.getBottom() - 15), 40, 12, juce::Justification::centred);
        g.drawText("10kHz", static_cast<int>(freqToX(10000.0f, bounds.getWidth()) - 20), static_cast<int>(bounds.getBottom() - 15), 40, 12, juce::Justification::centred);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() <= 0.0f) return;

        float f1 = getParamSafe("sub_low_freq", 90.0f);
        float f2 = getParamSafe("low_mid_freq", 500.0f);
        float f3 = getParamSafe("mid_high_freq", 1200.0f);
        float f4 = getParamSafe("high_super_freq", 5000.0f);

        float x1 = freqToX(f1, bounds.getWidth());
        float x2 = freqToX(f2, bounds.getWidth());
        float x3 = freqToX(f3, bounds.getWidth());
        float x4 = freqToX(f4, bounds.getWidth());

        int prevHover = hoveredSplitter;
        if (std::abs(e.position.x - x1) < 14.0f)
        {
            hoveredSplitter = 1;
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        }
        else if (std::abs(e.position.x - x2) < 14.0f)
        {
            hoveredSplitter = 2;
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        }
        else if (std::abs(e.position.x - x3) < 14.0f)
        {
            hoveredSplitter = 3;
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        }
        else if (std::abs(e.position.x - x4) < 14.0f)
        {
            hoveredSplitter = 4;
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        }
        else
        {
            hoveredSplitter = 0;
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }

        if (hoveredSplitter != prevHover)
            repaint();
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoveredSplitter != 0 && draggingSplitter == 0)
        {
            hoveredSplitter = 0;
            setMouseCursor(juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() <= 0.0f) return;

        float f1 = getParamSafe("sub_low_freq", 90.0f);
        float f2 = getParamSafe("low_mid_freq", 500.0f);
        float f3 = getParamSafe("mid_high_freq", 1200.0f);
        float f4 = getParamSafe("high_super_freq", 5000.0f);

        float x1 = freqToX(f1, bounds.getWidth());
        float x2 = freqToX(f2, bounds.getWidth());
        float x3 = freqToX(f3, bounds.getWidth());
        float x4 = freqToX(f4, bounds.getWidth());

        if (std::abs(e.position.x - x1) < 16.0f)
            draggingSplitter = 1;
        else if (std::abs(e.position.x - x2) < 16.0f)
            draggingSplitter = 2;
        else if (std::abs(e.position.x - x3) < 16.0f)
            draggingSplitter = 3;
        else if (std::abs(e.position.x - x4) < 16.0f)
            draggingSplitter = 4;

        if (draggingSplitter != 0)
            repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (draggingSplitter == 0 || bounds.getWidth() <= 0.0f) return;

        float newFreq = xToFreq(e.position.x, bounds.getWidth());

        if (draggingSplitter == 1)
        {
            float f2 = getParamSafe("low_mid_freq", 500.0f);
            newFreq = juce::jlimit(30.0f, f2 * 0.85f, newFreq);
            setParamSafe("sub_low_freq", newFreq);
        }
        else if (draggingSplitter == 2)
        {
            float f1 = getParamSafe("sub_low_freq", 90.0f);
            float f3 = getParamSafe("mid_high_freq", 1200.0f);
            newFreq = juce::jlimit(f1 * 1.15f, f3 * 0.85f, newFreq);
            setParamSafe("low_mid_freq", newFreq);
        }
        else if (draggingSplitter == 3)
        {
            float f2 = getParamSafe("low_mid_freq", 500.0f);
            float f4 = getParamSafe("high_super_freq", 5000.0f);
            newFreq = juce::jlimit(f2 * 1.15f, f4 * 0.85f, newFreq);
            setParamSafe("mid_high_freq", newFreq);
        }
        else if (draggingSplitter == 4)
        {
            float f3 = getParamSafe("mid_high_freq", 1200.0f);
            newFreq = juce::jlimit(f3 * 1.15f, 18000.0f, newFreq);
            setParamSafe("high_super_freq", newFreq);
        }

        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggingSplitter = 0;
        repaint();
    }

private:
    float getParamSafe(const juce::String& paramId, float fallback)
    {
        if (auto* param = apvts.getRawParameterValue(paramId))
            return param->load();
        return fallback;
    }

    void setParamSafe(const juce::String& paramId, float val)
    {
        if (auto* param = apvts.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    }

    float freqToX(float freq, float width) const
    {
        freq = juce::jlimit(20.0f, 20000.0f, freq);
        float norm = std::log10(freq / 20.0f) / std::log10(20000.0f / 20.0f);
        return norm * width;
    }

    float xToFreq(float x, float width) const
    {
        if (width <= 0.0f) return 1000.0f;
        float norm = juce::jlimit(0.0f, 1.0f, x / width);
        return 20.0f * std::pow(20000.0f / 20.0f, norm);
    }

    float dbToY(float db, float height) const
    {
        float norm = juce::jlimit(0.0f, 1.0f, (db + 30.0f) / 36.0f);
        return (height - 18.0f) * (1.0f - norm) + 6.0f;
    }

    void drawBandRegion(juce::Graphics& g, juce::Rectangle<float> bounds,
                        float startX, float endX, float grDb, juce::Colour colour,
                        const juce::String& name)
    {
        if (endX <= startX) return;

        float zeroY = dbToY(0.0f, bounds.getHeight());
        float dipY = dbToY(grDb, bounds.getHeight());

        juce::Path p;
        p.startNewSubPath(startX, bounds.getBottom() - 16.0f);
        p.lineTo(startX, zeroY);
        p.quadraticTo((startX + endX) * 0.5f, dipY, endX, zeroY);
        p.lineTo(endX, bounds.getBottom() - 16.0f);
        p.closeSubPath();

        // Shaded fill
        g.setColour(colour.withAlpha(0.18f));
        g.fillPath(p);

        // Top line
        juce::Path topCurve;
        topCurve.startNewSubPath(startX, zeroY);
        topCurve.quadraticTo((startX + endX) * 0.5f, dipY, endX, zeroY);
        g.setColour(colour.withAlpha(0.85f));
        g.strokePath(topCurve, juce::PathStrokeType(2.0f));

        // Name tag
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.setColour(colour.withAlpha(0.9f));
        float midX = (startX + endX) * 0.5f;
        g.drawText(name, static_cast<int>(midX - 50.0f), static_cast<int>(bounds.getY() + 8.0f),
                   100, 16, juce::Justification::centred);

        // GR Tag
        if (std::abs(grDb) > 0.1f)
        {
            g.setFont(juce::FontOptions(10.0f));
            g.setColour(juce::Colour(0xffff7675));
            juce::String grText = juce::String(grDb, 1) + " dB";
            g.drawText(grText, static_cast<int>(midX - 40.0f), static_cast<int>(dipY - 14.0f),
                       80, 14, juce::Justification::centred);
        }
    }

    void drawSplitter(juce::Graphics& g, juce::Rectangle<float> bounds,
                      float x, float freq, bool active, const juce::String& label)
    {
        juce::ignoreUnused(label);
        juce::Colour col = active ? juce::Colour(0xff00d2d3) : juce::Colour(0xff4cd137);

        // Glow
        if (active)
        {
            g.setColour(col.withAlpha(0.3f));
            g.fillRect(x - 5.0f, bounds.getY() + 4.0f, 10.0f, bounds.getHeight() - 20.0f);
        }

        // Line
        g.setColour(col);
        g.drawLine(x, bounds.getY() + 4.0f, x, bounds.getBottom() - 16.0f, active ? 2.5f : 1.5f);

        // Handle tag
        juce::Rectangle<float> tag(x - 28.0f, bounds.getBottom() - 34.0f, 56.0f, 16.0f);
        g.setColour(juce::Colour(0xff1e2229));
        g.fillRoundedRectangle(tag, 3.0f);
        g.setColour(col);
        g.drawRoundedRectangle(tag, 3.0f, 1.0f);

        juce::String fStr;
        if (freq >= 1000.0f)
            fStr = juce::String(freq / 1000.0f, 2) + "k";
        else
            fStr = juce::String(static_cast<int>(freq)) + "Hz";

        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(fStr, tag.toNearestInt(), juce::Justification::centred);
    }

    juce::AudioProcessorValueTreeState& apvts;
    float grDb1 = 0.0f;
    float grDb2 = 0.0f;
    float grDb3 = 0.0f;
    float grDb4 = 0.0f;
    float grDb5 = 0.0f;

    int hoveredSplitter = 0;  // 1..4
    int draggingSplitter = 0; // 1..4
};
