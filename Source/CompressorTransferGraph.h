#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

/**
 * Yamaha dspMixFx Compressor Transfer Curve Visualizer
 * Displays the Input/Output compression curve, Threshold ("T") node,
 * Knee curvature, Ratio ("R") node, and a dedicated vertical Gain Reduction (GR) meter.
 */
class CompressorTransferGraph : public juce::Component
{
public:
    CompressorTransferGraph(juce::Colour bandColour, std::function<void(float)> onThresholdChangedCallback = nullptr)
        : accent(bandColour), onThresholdChanged(onThresholdChangedCallback)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void setParameters(float threshold, float ratio, float knee, float grDb)
    {
        if (std::isnan(threshold) || std::isinf(threshold)) threshold = -18.0f;
        if (std::isnan(ratio) || std::isinf(ratio)) ratio = 3.2f;
        if (std::isnan(knee) || std::isinf(knee)) knee = 12.0f;
        if (std::isnan(grDb) || std::isinf(grDb)) grDb = 0.0f;

        float clampedTh = juce::jlimit(-60.0f, 0.0f, threshold);
        float clampedRat = juce::jmax(1.01f, ratio);
        float clampedKnee = juce::jmax(0.0f, knee);
        float clampedGr = juce::jlimit(-60.0f, 0.0f, grDb);

        if (std::abs(currentThreshold - clampedTh) < 0.1f &&
            std::abs(currentRatio - clampedRat) < 0.05f &&
            std::abs(currentKnee - clampedKnee) < 0.1f &&
            std::abs(currentGr - clampedGr) < 0.1f)
            return;

        currentThreshold = clampedTh;
        currentRatio = clampedRat;
        currentKnee = clampedKnee;
        currentGr = clampedGr;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() < 50.0f || bounds.getHeight() < 40.0f)
            return; // Safety guard for uninitialized, 0x0, or negative bounds in EqAPO

        // Layout: Left area = Curve Graph, Right area = Vertical GR Meter
        const float meterWidth = 36.0f;
        auto graphArea = bounds.removeFromLeft(juce::jmax(10.0f, bounds.getWidth() - meterWidth)).reduced(3.0f);
        auto meterArea = bounds.reduced(2.0f);

        // --- 1. DRAW GRAPH BOX ---
        g.setColour(juce::Colour(0xff121519));
        g.fillRect(graphArea);

        // Graph grid lines (-12, -24, -36, -48 dB)
        g.setColour(juce::Colour(0xff1f242c));
        for (float db = -48.0f; db <= -12.0f; db += 12.0f)
        {
            float x = dbToGraphX(db, graphArea);
            float y = dbToGraphY(db, graphArea);
            g.drawVerticalLine(static_cast<int>(x), graphArea.getY(), graphArea.getBottom());
            g.drawHorizontalLine(static_cast<int>(y), graphArea.getX(), graphArea.getRight());
        }

        // Compute transfer curve path
        juce::Path curvePath;
        juce::Path fillPath;

        const float minDb = -60.0f;
        const float maxDb = 0.0f;
        const int steps = 60;

        float startX = dbToGraphX(minDb, graphArea);
        float startY = dbToGraphY(computeOutputDb(minDb), graphArea);

        curvePath.startNewSubPath(startX, startY);
        fillPath.startNewSubPath(startX, graphArea.getBottom());
        fillPath.lineTo(startX, startY);

        for (int i = 1; i <= steps; ++i)
        {
            float inDb = minDb + (static_cast<float>(i) / static_cast<float>(steps)) * (maxDb - minDb);
            float outDb = computeOutputDb(inDb);

            float px = dbToGraphX(inDb, graphArea);
            float py = dbToGraphY(outDb, graphArea);

            curvePath.lineTo(px, py);
            fillPath.lineTo(px, py);
        }

        fillPath.lineTo(graphArea.getRight(), graphArea.getBottom());
        fillPath.closeSubPath();

        // Area fill under curve with translucent accent gradient
        juce::ColourGradient fillGrad(accent.withAlpha(0.35f), graphArea.getX(), graphArea.getY(),
                                      accent.withAlpha(0.08f), graphArea.getX(), graphArea.getBottom(), false);
        g.setGradientFill(fillGrad);
        g.fillPath(fillPath);

        // Curve stroke
        g.setColour(accent);
        g.strokePath(curvePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Outer graph box frame
        g.setColour(juce::Colour(0xff2d3542));
        g.drawRect(graphArea, 1.2f);

        // --- 2. DRAW "T" (THRESHOLD) AND "R" (RATIO) NODES ---
        // Threshold Node "T"
        float tX = dbToGraphX(currentThreshold, graphArea);
        float tY = dbToGraphY(computeOutputDb(currentThreshold), graphArea);

        // Node "T" shadow & circle
        const float tRadius = 7.5f;
        auto tRect = juce::Rectangle<float>(tX - tRadius, tY - tRadius, tRadius * 2.0f, tRadius * 2.0f);

        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillEllipse(tRect.translated(0.0f, 1.5f));

        g.setColour(isDraggingT ? juce::Colour(0xffffffaa) : juce::Colours::white);
        g.fillEllipse(tRect);

        g.setColour(juce::Colour(0xff1a1d22));
        g.drawEllipse(tRect, 1.2f);

        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.setColour(juce::Colours::black);
        g.drawText("T", tRect, juce::Justification::centred, false);

        // Ratio Node "R" at 0 dB input
        float rX = dbToGraphX(0.0f, graphArea);
        float rY = dbToGraphY(computeOutputDb(0.0f), graphArea);

        const float rRadius = 6.5f;
        auto rRect = juce::Rectangle<float>(rX - rRadius - 1.0f, rY - rRadius, rRadius * 2.0f, rRadius * 2.0f);

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillEllipse(rRect.translated(0.0f, 1.0f));

        g.setColour(juce::Colour(0xff555f6d));
        g.fillEllipse(rRect);

        g.setColour(juce::Colour(0xff2c313a));
        g.drawEllipse(rRect, 1.0f);

        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText("R", rRect, juce::Justification::centred, false);

        // --- 3. DRAW VERTICAL GAIN REDUCTION (GR) METER ---
        float barX = meterArea.getX() + 6.0f;
        float barW = 5.0f;
        float barTopY = graphArea.getY();
        float barH = graphArea.getHeight();

        // Meter background groove
        auto barRect = juce::Rectangle<float>(barX, barTopY, barW, barH);
        g.setColour(juce::Colour(0xff121519));
        g.fillRoundedRectangle(barRect, 2.0f);
        g.setColour(juce::Colour(0xff282f3a));
        g.drawRoundedRectangle(barRect, 2.0f, 1.0f);

        // Active Gain Reduction fill (drops downwards from top: 0dB down to -60dB)
        float grVal = juce::jlimit(0.0f, 60.0f, std::abs(currentGr));
        if (grVal > 0.1f)
        {
            float fillH = (grVal / 60.0f) * barH;
            auto activeFill = juce::Rectangle<float>(barX + 0.5f, barTopY + 0.5f, barW - 1.0f, fillH);

            juce::ColourGradient grGrad(accent.withMultipliedBrightness(1.2f), barX, barTopY,
                                        accent.withMultipliedBrightness(0.6f), barX, barTopY + fillH, false);
            g.setGradientFill(grGrad);
            g.fillRoundedRectangle(activeFill, 1.5f);
        }

        // Meter dB text labels (0, -6, -12, -18, -30, -60)
        float textX = barX + barW + 3.0f;
        float textW = meterArea.getRight() - textX;

        g.setFont(juce::FontOptions(8.5f));
        g.setColour(juce::Colour(0xff758296));

        auto drawDbLabel = [&](float db, const juce::String& text)
        {
            float yNorm = (std::abs(db) / 60.0f) * barH;
            float textY = barTopY + yNorm - 6.0f;
            g.drawText(text, static_cast<int>(textX), static_cast<int>(textY), static_cast<int>(textW), 12, juce::Justification::centredLeft);
        };

        drawDbLabel(0.0f, "0");
        drawDbLabel(-6.0f, "-6");
        drawDbLabel(-12.0f, "-12");
        drawDbLabel(-18.0f, "-18");
        drawDbLabel(-30.0f, "-30");
        drawDbLabel(-60.0f, "-60");

        // "GR" label under the meter
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xff8c9cb0));
        g.drawText("GR", static_cast<int>(barX - 2.0f), static_cast<int>(barTopY + barH + 2.0f), 24, 14, juce::Justification::centred);
    }

    // --- MOUSE INTERACTION TO DRAG THRESHOLD "T" NODE ---
    void mouseMove(const juce::MouseEvent& e) override
    {
        auto graphArea = getGraphArea();
        if (graphArea.getWidth() < 20.0f || graphArea.getHeight() < 20.0f)
            return;

        float tX = dbToGraphX(currentThreshold, graphArea);
        float tY = dbToGraphY(computeOutputDb(currentThreshold), graphArea);

        bool isOverT = e.position.getDistanceFrom(juce::Point<float>(tX, tY)) < 12.0f;
        setMouseCursor(isOverT ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto graphArea = getGraphArea();
        if (graphArea.getWidth() < 20.0f || graphArea.getHeight() < 20.0f)
            return;

        float tX = dbToGraphX(currentThreshold, graphArea);
        float tY = dbToGraphY(computeOutputDb(currentThreshold), graphArea);

        if (e.position.getDistanceFrom(juce::Point<float>(tX, tY)) < 16.0f)
        {
            isDraggingT = true;
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isDraggingT && onThresholdChanged != nullptr)
        {
            auto graphArea = getGraphArea();
            if (graphArea.getWidth() < 20.0f)
                return;

            float normX = juce::jlimit(0.0f, 1.0f, (e.position.x - graphArea.getX()) / graphArea.getWidth());
            float newThreshold = -60.0f + normX * 60.0f; // -60 to 0 dB
            newThreshold = juce::jlimit(-60.0f, 0.0f, newThreshold);

            onThresholdChanged(newThreshold);
            currentThreshold = newThreshold;
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        isDraggingT = false;
        repaint();
    }

private:
    juce::Rectangle<float> getGraphArea() const
    {
        auto b = getLocalBounds().toFloat();
        if (b.getWidth() < 50.0f || b.getHeight() < 40.0f)
            return juce::Rectangle<float>();

        const float meterWidth = 36.0f;
        return b.removeFromLeft(juce::jmax(10.0f, b.getWidth() - meterWidth)).reduced(3.0f);
    }

    float dbToGraphX(float db, juce::Rectangle<float> rect) const
    {
        if (rect.getWidth() <= 0.0f) return rect.getX();
        float norm = juce::jlimit(0.0f, 1.0f, (db - (-60.0f)) / 60.0f);
        return rect.getX() + norm * rect.getWidth();
    }

    float dbToGraphY(float db, juce::Rectangle<float> rect) const
    {
        if (rect.getHeight() <= 0.0f) return rect.getBottom();
        float norm = juce::jlimit(0.0f, 1.0f, (db - (-60.0f)) / 60.0f);
        return rect.getBottom() - norm * rect.getHeight();
    }

    float computeOutputDb(float inDb) const
    {
        float ratioSafe = juce::jmax(1.01f, currentRatio);
        float kneeSafe = juce::jmax(0.0f, currentKnee);
        float delta = inDb - currentThreshold;

        if (kneeSafe <= 0.1f)
        {
            // Hard knee
            if (delta <= 0.0f) return inDb;
            return currentThreshold + (delta / ratioSafe);
        }

        // Soft knee
        if (delta <= -kneeSafe / 2.0f)
        {
            return inDb;
        }
        else if (delta >= kneeSafe / 2.0f)
        {
            return currentThreshold + (delta / ratioSafe);
        }
        else
        {
            float x = delta + (kneeSafe / 2.0f);
            float gr = (x * x / (2.0f * kneeSafe)) * (1.0f - (1.0f / ratioSafe));
            return inDb - gr;
        }
    }

    juce::Colour accent;
    std::function<void(float)> onThresholdChanged;

    float currentThreshold = -18.0f;
    float currentRatio = 3.2f;
    float currentKnee = 12.0f;
    float currentGr = 0.0f;

    bool isDraggingT = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorTransferGraph)
};
