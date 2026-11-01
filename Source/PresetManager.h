#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <map>
#include <vector>

/**
 * Ultra-Fast In-Memory & JSON Hybrid Preset Manager for 5 Multiband Compressor.
 * 
 * Performance & Stability Architecture:
 * 1. Factory presets are held in-memory (0ms startup, zero disk I/O, zero antivirus lock).
 * 2. User-created presets are saved/loaded on-demand from %APPDATA%/5 Multiband Compressor/Presets/
 * 3. 100% immune to Equalizer APO and headless DAW scanning hangs.
 */
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& stateToUse)
        : apvts(stateToUse)
    {
    }

    static const std::vector<juce::String>& getFactoryPresetNames()
    {
        static const std::vector<juce::String> names = {
            "1. Gamelan Karawitan (Megah & Alus)",
            "2. Sound Horeg (Glerr & Nendang)",
            "3. Dangdut Koplo Lapangan",
            "4. Air & Silky Highs",
            "5. Tube Amplifier Warmth",
            "6. Vintage Jazz (Warm Opto)",
            "7. Punchy Master (Modern Pop & EDM)",
            "8. Vocal Polish & De-Esser",
            "9. Acoustic & Bus Glue"
        };
        return names;
    }

    static const std::map<juce::String, float>* getFactoryPresetParams(const juce::String& name)
    {
        static const std::map<juce::String, std::map<juce::String, float>> factoryPresets = {
            {
                "1. Gamelan Karawitan (Megah & Alus)",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 80.0f}, {"low_mid_freq", 450.0f}, {"mid_high_freq", 1500.0f}, {"high_super_freq", 5000.0f},
                    {"sub_peak_reduction", 32.0f}, {"sub_gain", 1.0f}, {"sub_warmth", 25.0f}, {"sub_threshold", -16.0f}, {"sub_ratio", 3.0f}, {"sub_attack", 35.0f}, {"sub_release", 350.0f}, {"sub_knee", 12.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 28.0f}, {"low_gain", 0.5f}, {"low_warmth", 25.0f}, {"low_threshold", -15.0f}, {"low_ratio", 2.8f}, {"low_attack", 25.0f}, {"low_release", 200.0f}, {"low_knee", 12.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 25.0f}, {"mid_gain", 0.0f}, {"mid_warmth", 20.0f}, {"mid_threshold", -15.0f}, {"mid_ratio", 2.5f}, {"mid_attack", 15.0f}, {"mid_release", 120.0f}, {"mid_knee", 12.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 42.0f}, {"mid_high_gain", -0.5f}, {"mid_high_warmth", 35.0f}, {"mid_high_threshold", -20.0f}, {"mid_high_ratio", 3.2f}, {"mid_high_attack", 10.0f}, {"mid_high_release", 90.0f}, {"mid_high_knee", 14.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 24.0f}, {"high_gain", 1.0f}, {"high_warmth", 25.0f}, {"high_threshold", -14.0f}, {"high_ratio", 2.2f}, {"high_attack", 5.0f}, {"high_release", 60.0f}, {"high_knee", 16.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            },
            {
                "2. Sound Horeg (Glerr & Nendang)",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 75.0f}, {"low_mid_freq", 450.0f}, {"mid_high_freq", 2200.0f}, {"high_super_freq", 6000.0f},
                    {"sub_peak_reduction", 55.0f}, {"sub_gain", 3.5f}, {"sub_warmth", 35.0f}, {"sub_threshold", -24.0f}, {"sub_ratio", 6.0f}, {"sub_attack", 20.0f}, {"sub_release", 180.0f}, {"sub_knee", 12.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 40.0f}, {"low_gain", 1.5f}, {"low_warmth", 20.0f}, {"low_threshold", -18.0f}, {"low_ratio", 4.0f}, {"low_attack", 15.0f}, {"low_release", 120.0f}, {"low_knee", 12.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 25.0f}, {"mid_gain", -0.5f}, {"mid_warmth", 15.0f}, {"mid_threshold", -15.0f}, {"mid_ratio", 3.0f}, {"mid_attack", 10.0f}, {"mid_release", 100.0f}, {"mid_knee", 12.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 35.0f}, {"mid_high_gain", 2.0f}, {"mid_high_warmth", 20.0f}, {"mid_high_threshold", -18.0f}, {"mid_high_ratio", 3.5f}, {"mid_high_attack", 5.0f}, {"mid_high_release", 80.0f}, {"mid_high_knee", 14.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 30.0f}, {"high_gain", 2.5f}, {"high_warmth", 25.0f}, {"high_threshold", -16.0f}, {"high_ratio", 2.8f}, {"high_attack", 3.0f}, {"high_release", 50.0f}, {"high_knee", 16.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            },
            {
                "3. Dangdut Koplo Lapangan",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 85.0f}, {"low_mid_freq", 500.0f}, {"mid_high_freq", 2000.0f}, {"high_super_freq", 5500.0f},
                    {"sub_peak_reduction", 45.0f}, {"sub_gain", 2.0f}, {"sub_warmth", 25.0f}, {"sub_threshold", -18.0f}, {"sub_ratio", 4.5f}, {"sub_attack", 25.0f}, {"sub_release", 150.0f}, {"sub_knee", 10.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 45.0f}, {"low_gain", 2.5f}, {"low_warmth", 30.0f}, {"low_threshold", -18.0f}, {"low_ratio", 4.0f}, {"low_attack", 15.0f}, {"low_release", 120.0f}, {"low_knee", 12.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 30.0f}, {"mid_gain", -1.0f}, {"mid_warmth", 15.0f}, {"mid_threshold", -16.0f}, {"mid_ratio", 3.0f}, {"mid_attack", 10.0f}, {"mid_release", 90.0f}, {"mid_knee", 12.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 38.0f}, {"mid_high_gain", 2.5f}, {"mid_high_warmth", 20.0f}, {"mid_high_threshold", -18.0f}, {"mid_high_ratio", 3.2f}, {"mid_high_attack", 5.0f}, {"mid_high_release", 70.0f}, {"mid_high_knee", 14.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 25.0f}, {"high_gain", 1.8f}, {"high_warmth", 20.0f}, {"high_threshold", -16.0f}, {"high_ratio", 2.5f}, {"high_attack", 3.0f}, {"high_release", 50.0f}, {"high_knee", 16.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            },
            {
                "4. Air & Silky Highs",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 90.0f}, {"low_mid_freq", 500.0f}, {"mid_high_freq", 1200.0f}, {"high_super_freq", 5000.0f},
                    {"sub_peak_reduction", 30.0f}, {"sub_gain", 0.0f}, {"sub_warmth", 15.0f}, {"sub_threshold", -18.0f}, {"sub_ratio", 3.0f}, {"sub_attack", 30.0f}, {"sub_release", 200.0f}, {"sub_knee", 10.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 25.0f}, {"low_gain", 0.0f}, {"low_warmth", 15.0f}, {"low_threshold", -16.0f}, {"low_ratio", 2.8f}, {"low_attack", 20.0f}, {"low_release", 150.0f}, {"low_knee", 12.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 25.0f}, {"mid_gain", 0.0f}, {"mid_warmth", 15.0f}, {"mid_threshold", -16.0f}, {"mid_ratio", 2.5f}, {"mid_attack", 10.0f}, {"mid_release", 100.0f}, {"mid_knee", 12.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 40.0f}, {"mid_high_gain", 0.5f}, {"mid_high_warmth", 30.0f}, {"mid_high_threshold", -20.0f}, {"mid_high_ratio", 3.0f}, {"mid_high_attack", 5.0f}, {"mid_high_release", 80.0f}, {"mid_high_knee", 14.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 20.0f}, {"high_gain", 3.0f}, {"high_warmth", 40.0f}, {"high_threshold", -14.0f}, {"high_ratio", 2.0f}, {"high_attack", 3.0f}, {"high_release", 50.0f}, {"high_knee", 16.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            },
            {
                "5. Tube Amplifier Warmth",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 90.0f}, {"low_mid_freq", 500.0f}, {"mid_high_freq", 1200.0f}, {"high_super_freq", 5000.0f},
                    {"sub_peak_reduction", 25.0f}, {"sub_gain", 0.5f}, {"sub_warmth", 50.0f}, {"sub_threshold", -16.0f}, {"sub_ratio", 3.0f}, {"sub_attack", 30.0f}, {"sub_release", 200.0f}, {"sub_knee", 12.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 30.0f}, {"low_gain", 0.5f}, {"low_warmth", 50.0f}, {"low_threshold", -16.0f}, {"low_ratio", 3.0f}, {"low_attack", 20.0f}, {"low_release", 150.0f}, {"low_knee", 12.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 30.0f}, {"mid_gain", 0.5f}, {"mid_warmth", 45.0f}, {"mid_threshold", -16.0f}, {"mid_ratio", 2.8f}, {"mid_attack", 10.0f}, {"mid_release", 100.0f}, {"mid_knee", 12.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 28.0f}, {"mid_high_gain", 0.0f}, {"mid_high_warmth", 45.0f}, {"mid_high_threshold", -16.0f}, {"mid_high_ratio", 2.5f}, {"mid_high_attack", 5.0f}, {"mid_high_release", 80.0f}, {"mid_high_knee", 14.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 25.0f}, {"high_gain", 0.5f}, {"high_warmth", 40.0f}, {"high_threshold", -16.0f}, {"high_ratio", 2.2f}, {"high_attack", 3.0f}, {"high_release", 50.0f}, {"high_knee", 16.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            },
            {
                "6. Vintage Jazz (Warm Opto)",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 90.0f}, {"low_mid_freq", 480.0f}, {"mid_high_freq", 1200.0f}, {"high_super_freq", 4800.0f},
                    {"sub_peak_reduction", 35.0f}, {"sub_gain", 0.5f}, {"sub_warmth", 30.0f}, {"sub_threshold", -18.0f}, {"sub_ratio", 2.8f}, {"sub_attack", 35.0f}, {"sub_release", 250.0f}, {"sub_knee", 14.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 30.0f}, {"low_gain", 0.0f}, {"low_warmth", 25.0f}, {"low_threshold", -16.0f}, {"low_ratio", 2.5f}, {"low_attack", 25.0f}, {"low_release", 180.0f}, {"low_knee", 14.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 28.0f}, {"mid_gain", 0.0f}, {"mid_warmth", 25.0f}, {"mid_threshold", -16.0f}, {"mid_ratio", 2.5f}, {"mid_attack", 15.0f}, {"mid_release", 120.0f}, {"mid_knee", 14.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 30.0f}, {"mid_high_gain", 0.0f}, {"mid_high_warmth", 30.0f}, {"mid_high_threshold", -16.0f}, {"mid_high_ratio", 2.2f}, {"mid_high_attack", 8.0f}, {"mid_high_release", 90.0f}, {"mid_high_knee", 16.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 22.0f}, {"high_gain", 0.5f}, {"high_warmth", 30.0f}, {"high_threshold", -14.0f}, {"high_ratio", 2.0f}, {"high_attack", 5.0f}, {"high_release", 60.0f}, {"high_knee", 18.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            },
            {
                "7. Punchy Master (Modern Pop & EDM)",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 90.0f}, {"low_mid_freq", 500.0f}, {"mid_high_freq", 1200.0f}, {"high_super_freq", 5000.0f},
                    {"sub_peak_reduction", 45.0f}, {"sub_gain", 0.0f}, {"sub_warmth", 20.0f}, {"sub_threshold", -20.0f}, {"sub_ratio", 5.0f}, {"sub_attack", 25.0f}, {"sub_release", 160.0f}, {"sub_knee", 10.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 38.0f}, {"low_gain", 0.5f}, {"low_warmth", 15.0f}, {"low_threshold", -18.0f}, {"low_ratio", 4.0f}, {"low_attack", 15.0f}, {"low_release", 120.0f}, {"low_knee", 10.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 32.0f}, {"mid_gain", 0.0f}, {"mid_warmth", 15.0f}, {"mid_threshold", -16.0f}, {"mid_ratio", 3.0f}, {"mid_attack", 10.0f}, {"mid_release", 90.0f}, {"mid_knee", 12.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 28.0f}, {"mid_high_gain", 1.0f}, {"mid_high_warmth", 15.0f}, {"mid_high_threshold", -16.0f}, {"mid_high_ratio", 2.8f}, {"mid_high_attack", 5.0f}, {"mid_high_release", 70.0f}, {"mid_high_knee", 14.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 22.0f}, {"high_gain", 1.5f}, {"high_warmth", 20.0f}, {"high_threshold", -14.0f}, {"high_ratio", 2.2f}, {"high_attack", 3.0f}, {"high_release", 50.0f}, {"high_knee", 16.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            },
            {
                "8. Vocal Polish & De-Esser",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 100.0f}, {"low_mid_freq", 500.0f}, {"mid_high_freq", 1400.0f}, {"high_super_freq", 5500.0f},
                    {"sub_peak_reduction", 40.0f}, {"sub_gain", -1.5f}, {"sub_warmth", 10.0f}, {"sub_threshold", -24.0f}, {"sub_ratio", 4.0f}, {"sub_attack", 15.0f}, {"sub_release", 120.0f}, {"sub_knee", 10.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 30.0f}, {"low_gain", 0.0f}, {"low_warmth", 25.0f}, {"low_threshold", -18.0f}, {"low_ratio", 3.0f}, {"low_attack", 15.0f}, {"low_release", 120.0f}, {"low_knee", 12.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 35.0f}, {"mid_gain", 1.0f}, {"mid_warmth", 20.0f}, {"mid_threshold", -16.0f}, {"mid_ratio", 3.0f}, {"mid_attack", 8.0f}, {"mid_release", 90.0f}, {"mid_knee", 12.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 48.0f}, {"mid_high_gain", 0.0f}, {"mid_high_warmth", 25.0f}, {"mid_high_threshold", -22.0f}, {"mid_high_ratio", 3.8f}, {"mid_high_attack", 3.0f}, {"mid_high_release", 60.0f}, {"mid_high_knee", 16.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 25.0f}, {"high_gain", 1.5f}, {"high_warmth", 30.0f}, {"high_threshold", -14.0f}, {"high_ratio", 2.2f}, {"high_attack", 3.0f}, {"high_release", 50.0f}, {"high_knee", 16.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            },
            {
                "9. Acoustic & Bus Glue",
                {
                    {"ui_mode", 0.0f}, {"oversampling", 0.0f},
                    {"sub_low_freq", 90.0f}, {"low_mid_freq", 500.0f}, {"mid_high_freq", 1200.0f}, {"high_super_freq", 5000.0f},
                    {"sub_peak_reduction", 20.0f}, {"sub_gain", 0.0f}, {"sub_warmth", 15.0f}, {"sub_threshold", -14.0f}, {"sub_ratio", 2.2f}, {"sub_attack", 30.0f}, {"sub_release", 200.0f}, {"sub_knee", 14.0f}, {"sub_engine", 1.0f}, {"sub_band_mix", 100.0f}, {"sub_bypass", 0.0f}, {"sub_solo", 0.0f}, {"sub_mute", 0.0f}, {"sub_mode", 0.0f},
                    {"low_peak_reduction", 25.0f}, {"low_gain", 0.0f}, {"low_warmth", 15.0f}, {"low_threshold", -14.0f}, {"low_ratio", 2.2f}, {"low_attack", 20.0f}, {"low_release", 150.0f}, {"low_knee", 14.0f}, {"low_engine", 1.0f}, {"low_band_mix", 100.0f}, {"low_bypass", 0.0f}, {"low_solo", 0.0f}, {"low_mute", 0.0f}, {"low_mode", 0.0f},
                    {"mid_peak_reduction", 25.0f}, {"mid_gain", 0.0f}, {"mid_warmth", 15.0f}, {"mid_threshold", -14.0f}, {"mid_ratio", 2.0f}, {"mid_attack", 12.0f}, {"mid_release", 100.0f}, {"mid_knee", 14.0f}, {"mid_engine", 1.0f}, {"mid_band_mix", 100.0f}, {"mid_bypass", 0.0f}, {"mid_solo", 0.0f}, {"mid_mute", 0.0f}, {"mid_mode", 0.0f},
                    {"mid_high_peak_reduction", 22.0f}, {"mid_high_gain", 0.0f}, {"mid_high_warmth", 15.0f}, {"mid_high_threshold", -14.0f}, {"mid_high_ratio", 2.0f}, {"mid_high_attack", 8.0f}, {"mid_high_release", 80.0f}, {"mid_high_knee", 16.0f}, {"mid_high_engine", 1.0f}, {"mid_high_band_mix", 100.0f}, {"mid_high_bypass", 0.0f}, {"mid_high_solo", 0.0f}, {"mid_high_mute", 0.0f}, {"mid_high_mode", 0.0f},
                    {"high_peak_reduction", 18.0f}, {"high_gain", 0.5f}, {"high_warmth", 15.0f}, {"high_threshold", -12.0f}, {"high_ratio", 1.8f}, {"high_attack", 5.0f}, {"high_release", 60.0f}, {"high_knee", 18.0f}, {"high_engine", 1.0f}, {"high_band_mix", 100.0f}, {"high_bypass", 0.0f}, {"high_solo", 0.0f}, {"high_mute", 0.0f}, {"high_mode", 0.0f},
                    {"input_gain", 0.0f}, {"output_gain", 0.0f}, {"mix", 100.0f}
                }
            }
        };

        auto it = factoryPresets.find(name);
        if (it != factoryPresets.end())
            return &(it->second);

        return nullptr;
    }

    juce::File getPresetsDirectory() const
    {
        auto appDataStr = juce::SystemStats::getEnvironmentVariable("APPDATA", "");
        juce::File dir;
        if (appDataStr.isNotEmpty())
            dir = juce::File(appDataStr).getChildFile("5 Multiband Compressor").getChildFile("Presets");
        else
            dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("5 Multiband Compressor")
                       .getChildFile("Presets");

        if (!dir.exists())
            dir.createDirectory();
        return dir;
    }

    mutable juce::StringArray cachedPresets;
    mutable bool presetsNeedRefresh = true;

    juce::StringArray getAvailablePresets() const
    {
        if (!presetsNeedRefresh && cachedPresets.size() > 0)
            return cachedPresets;

        juce::StringArray list;
        // 1. Factory presets loaded instantly from in-memory index
        for (const auto& name : getFactoryPresetNames())
            list.add(name);

        // 2. Custom presets from disk (only check if directory already exists, zero disk creation)
        auto appDataStr = juce::SystemStats::getEnvironmentVariable("APPDATA", "");
        juce::File dir;
        if (appDataStr.isNotEmpty())
            dir = juce::File(appDataStr).getChildFile("5 Multiband Compressor").getChildFile("Presets");
        else
            dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("5 Multiband Compressor")
                       .getChildFile("Presets");

        if (dir.exists())
        {
            auto files = dir.findChildFiles(juce::File::findFiles, false, "*.json");
            for (const auto& f : files)
            {
                auto name = f.getFileNameWithoutExtension();
                if (!list.contains(name))
                    list.add(name);
            }
        }

        cachedPresets = list;
        presetsNeedRefresh = false;
        return list;
    }

    bool savePreset(const juce::String& presetName)
    {
        if (presetName.trim().isEmpty())
            return false;

        auto safeName = juce::File::createLegalFileName(presetName.trim());
        auto file = getPresetsDirectory().getChildFile(safeName + ".json");

        juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();
        rootObj->setProperty("name", presetName.trim());
        rootObj->setProperty("version", 1);

        juce::DynamicObject::Ptr paramsObj = new juce::DynamicObject();
        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* pWithId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            {
                if (auto* raw = apvts.getRawParameterValue(pWithId->paramID))
                    paramsObj->setProperty(pWithId->paramID, raw->load());
            }
        }
        rootObj->setProperty("parameters", juce::var(paramsObj.get()));

        juce::String jsonText = juce::JSON::toString(juce::var(rootObj.get()), true);
        bool success = file.replaceWithText(jsonText);
        if (success) presetsNeedRefresh = true;
        return success;
    }

    bool loadPreset(const juce::String& presetName)
    {
        // 1. Check in-memory factory presets (fast path, zero disk I/O)
        if (const auto* factoryParams = getFactoryPresetParams(presetName.trim()))
        {
            for (const auto& kv : *factoryParams)
            {
                if (auto* param = apvts.getParameter(kv.first))
                {
                    param->setValueNotifyingHost(param->convertTo0to1(kv.second));
                }
            }
            return true;
        }

        // 2. Custom disk preset
        auto safeName = juce::File::createLegalFileName(presetName.trim());
        auto file = getPresetsDirectory().getChildFile(safeName + ".json");
        if (!file.existsAsFile())
            return false;

        return loadPresetFromFile(file);
    }

    bool loadPresetFromFile(const juce::File& file)
    {
        if (!file.existsAsFile())
            return false;

        auto parsed = juce::JSON::parse(file.loadFileAsString());
        if (!parsed.isObject())
            return false;

        auto* rootObj = parsed.getDynamicObject();
        if (rootObj == nullptr || !rootObj->hasProperty("parameters"))
            return false;

        auto* paramsObj = rootObj->getProperty("parameters").getDynamicObject();
        if (paramsObj == nullptr)
            return false;

        for (const auto& prop : paramsObj->getProperties())
        {
            juce::String paramID = prop.name.toString();
            float val = static_cast<float>(prop.value);

            if (auto* param = apvts.getParameter(paramID))
            {
                param->setValueNotifyingHost(param->convertTo0to1(val));
            }
        }

        return true;
    }

    // Deprecated eager file writer: preserved as a lightweight no-op to ensure zero startup latency
    void ensureFactoryPresets()
    {
        // No-op: all factory presets are served instantaneously from in-memory cache!
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
};
