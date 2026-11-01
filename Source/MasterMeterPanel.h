#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

//==============================================================================
/** Professional dual vertical stereo I/O dB meter with dynamic histogram history,
    peak-hold indicators, calibrated scale ticks, and clipping warnings. */
class MasterMeterPanel : public juce::Component
{
public:
    MasterMeterPanel()
    {
        for (int i = 0; i < HISTORY_SIZE; ++i)
        {
            inHistory[i] = -60.0f;
            outHistory[i] = -60.0f;
        }
    }

    void setLevels(float inDb, float outDb)
    {
        if (std::isnan(inDb) || std::isinf(inDb)) inDb = -60.0f;
        if (std::isnan(outDb) || std::isinf(outDb)) outDb = -60.0f;

        currentInDb = juce::jlimit(-60.0f, 12.0f, inDb);
        currentOutDb = juce::jlimit(-60.0f, 12.0f, outDb);

        // Peak Hold logic (hold for ~30 frames @ 30Hz = ~1 sec, then decay)
        if (currentInDb >= peakInDb)
        {
            peakInDb = currentInDb;
            peakInHoldTimer = 30;
        }
        else if (peakInHoldTimer > 0)
        {
            --peakInHoldTimer;
        }
        else
        {
            peakInDb = juce::jmax(currentInDb, peakInDb - 1.5f);
        }

        if (currentOutDb >= peakOutDb)
        {
            peakOutDb = currentOutDb;
            peakOutHoldTimer = 30;
        }
        else if (peakOutHoldTimer > 0)
        {
            --peakOutHoldTimer;
        }
        else
        {
            peakOutDb = juce::jmax(currentOutDb, peakOutDb - 1.5f);
        }

        // Circular histogram buffer
        inHistory[historyIndex] = currentInDb;
        outHistory[historyIndex] = currentOutDb;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() < 40.0f || bounds.getHeight() < 50.0f)
            return;

        // 1. Dark brushed display background matching CrossoverDisplay
        juce::ColourGradient bgGrad(juce::Colour(0xff121417), 0.0f, 0.0f,
                                    juce::Colour(0xff1a1d22), 0.0f, bounds.getHeight(), false);
        g.setGradientFill(bgGrad);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Outer metal chassis border
        g.setColour(juce::Colour(0xff2d333f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.2f);

        // Subtle top accent line
        g.setColour(juce::Colour(0xff00d2d3).withAlpha(0.45f));
        g.drawLine(bounds.getX() + 6.0f, bounds.getY() + 1.0f, bounds.getRight() - 6.0f, bounds.getY() + 1.0f, 1.2f);

        // 2. Title & Channel Labels
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xff00d2d3));
        g.drawText("I/O LEVELS", bounds.removeFromTop(18.0f), juce::Justification::centred, false);

        auto contentArea = bounds.reduced(6.0f, 2.0f);

        // Bottom numeric readout space
        auto numericArea = contentArea.removeFromBottom(16.0f);

        // Meter column layout: [IN BAR] [SCALE] [OUT BAR]
        const float totalW = contentArea.getWidth();
        const float scaleW = 34.0f;
        const float barW = (totalW - scaleW - 8.0f) / 2.0f;

        auto inBarBounds = juce::Rectangle<float>(contentArea.getX() + 2.0f, contentArea.getY(), barW, contentArea.getHeight());
        auto scaleBounds = juce::Rectangle<float>(inBarBounds.getRight() + 2.0f, contentArea.getY(), scaleW, contentArea.getHeight());
        auto outBarBounds = juce::Rectangle<float>(scaleBounds.getRight() + 2.0f, contentArea.getY(), barW, contentArea.getHeight());

        // Draw meter bar backgrounds & tracks
        auto drawTrack = [&g](juce::Rectangle<float> r)
        {
            g.setColour(juce::Colour(0xff0d0f12));
            g.fillRoundedRectangle(r, 2.5f);
            g.setColour(juce::Colour(0xff222731));
            g.drawRoundedRectangle(r, 2.5f, 1.0f);
        };
        drawTrack(inBarBounds);
        drawTrack(outBarBounds);

        // dB Scale Mapping: -60 dB to +6 dB
        const float minDb = -60.0f;
        const float maxDb = 6.0f;
        auto dbToY = [&](float db) -> float
        {
            float norm = (db - minDb) / (maxDb - minDb);
            norm = juce::jlimit(0.0f, 1.0f, norm);
            return inBarBounds.getBottom() - norm * inBarBounds.getHeight();
        };

        // Draw subtle horizontal calibration tick marks
        const float tickDbs[] = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f };
        g.setFont(juce::FontOptions(8.5f, juce::Font::plain));

        for (float db : tickDbs)
        {
            float y = dbToY(db);

            // Ticks across bars
            g.setColour(db == 0.0f ? juce::Colour(0xffe1e8f0).withAlpha(0.4f) : juce::Colour(0xff2d3542));
            g.drawHorizontalLine(static_cast<int>(y), inBarBounds.getX() + 1.0f, inBarBounds.getRight() - 1.0f);
            g.drawHorizontalLine(static_cast<int>(y), outBarBounds.getX() + 1.0f, outBarBounds.getRight() - 1.0f);

            // Scale text in center column
            g.setColour(db > 0.0f ? juce::Colour(0xffff5252) : (db == 0.0f ? juce::Colours::white : juce::Colour(0xff8c96a5)));
            juce::String txt = (db > 0.0f ? "+" : "") + juce::String(static_cast<int>(db));
            g.drawText(txt, juce::Rectangle<float>(scaleBounds.getX(), y - 6.0f, scaleBounds.getWidth(), 12.0f), juce::Justification::centred, false);
        }

        // Draw Active Level Bars (Gradient: Cyan/Green -> Yellow -> Red)
        auto drawLevelBar = [&](juce::Rectangle<float> r, float db, float peakDb, juce::Colour baseCol)
        {
            float y = dbToY(db);
            float barH = r.getBottom() - y;
            if (barH > 1.0f)
            {
                auto activeRect = juce::Rectangle<float>(r.getX() + 1.5f, y, r.getWidth() - 3.0f, barH);

                juce::ColourGradient grad(baseCol, activeRect.getX(), r.getBottom(),
                                          juce::Colour(0xffff3838), activeRect.getX(), r.getY(), false);
                grad.addColour(0.72f, juce::Colour(0xfff5cd79)); // ~ -6 to 0 dB
                grad.addColour(0.85f, juce::Colour(0xffff5252)); // 0 dB to +6 dB

                g.setGradientFill(grad);
                g.fillRoundedRectangle(activeRect, 2.0f);
            }

            // Peak Hold line
            if (peakDb > minDb + 1.0f)
            {
                float peakY = dbToY(peakDb);
                g.setColour(peakDb >= 0.0f ? juce::Colour(0xffff3838) : juce::Colours::white);
                g.drawHorizontalLine(static_cast<int>(peakY), r.getX() + 1.0f, r.getRight() - 1.0f);
            }

            // Clip LED (at the very top of each bar)
            bool isClipping = (peakDb >= 0.0f);
            auto clipBox = juce::Rectangle<float>(r.getCentreX() - 4.0f, r.getY() - 5.0f, 8.0f, 4.0f);
            g.setColour(isClipping ? juce::Colour(0xffff3838) : juce::Colour(0xff252a33));
            g.fillRoundedRectangle(clipBox, 1.5f);
            if (isClipping)
            {
                g.setColour(juce::Colour(0xffff3838).withAlpha(0.6f));
                g.drawRoundedRectangle(clipBox.expanded(1.0f), 2.0f, 1.0f);
            }
        };

        drawLevelBar(inBarBounds, currentInDb, peakInDb, juce::Colour(0xff00d2d3));
        drawLevelBar(outBarBounds, currentOutDb, peakOutDb, juce::Colour(0xff2ed573));

        // 3. Digital Readout at bottom
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        auto inNumArea = numericArea.removeFromLeft(totalW * 0.48f);
        auto outNumArea = numericArea.removeFromRight(totalW * 0.48f);

        juce::String inStr = currentInDb <= -59.5f ? "IN: -inf" : ("IN " + juce::String(currentInDb, 1));
        juce::String outStr = currentOutDb <= -59.5f ? "OUT: -inf" : ("OUT " + juce::String(currentOutDb, 1));

        g.setColour(currentInDb >= 0.0f ? juce::Colour(0xffff5252) : juce::Colour(0xff00d2d3));
        g.drawText(inStr, inNumArea, juce::Justification::centred, false);

        g.setColour(currentOutDb >= 0.0f ? juce::Colour(0xffff5252) : juce::Colour(0xff2ed573));
        g.drawText(outStr, outNumArea, juce::Justification::centred, false);
    }

private:
    float currentInDb = -60.0f;
    float currentOutDb = -60.0f;
    float peakInDb = -60.0f;
    float peakOutDb = -60.0f;
    int peakInHoldTimer = 0;
    int peakOutHoldTimer = 0;

    static constexpr int HISTORY_SIZE = 48;
    float inHistory[HISTORY_SIZE] = {};
    float outHistory[HISTORY_SIZE] = {};
    int historyIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterMeterPanel)
};
