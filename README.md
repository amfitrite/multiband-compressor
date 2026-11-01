# 5 Multiband Compressor

Vintage LA-2A Style Multiband Audio Plugin (JUCE / VST3 / Standalone / Equalizer APO).

## Features
- **5-Band Linkwitz-Riley Crossover**: 4th order (24 dB/oct) linear-phase aligned multiband crossover with adjustable split frequencies (Sub, Low, Mid, High, Air).
- **LA-2A Style Opto Dynamics**: Dual-stage non-linear T4 optical cell model per band with program-dependent attack and multi-stage exponential release.
- **Equalizer APO Live Sync**: Zero-latency in-RAM shared memory bridge (`Local\MB5B_EqApoLiveSync`) allowing smooth real-time parameter tweaking in headless or host environments without disk file locking.
- **Factory Presets**: Built-in in-memory presets for instant zero-latency loading:
  - *Broadcast Punch*
  - *Vocal Presence*
  - *Warm Bass Control*
  - *Master Glue*
  - *Acoustic Clarity*
  - *Drum Bus Tight*
  - *Sibilance Tamer*
  - *Transparent Leveling*
  - *Gentle Mastering*
- **Oversampling Engine**: High-fidelity oversampling (2x, 4x, 8x, 16x) with lazy allocation.
- **Visuals**: Real-time gain reduction meters, master I/O peak & RMS metering, and interactive crossover transfer curves.

## Building
- **JUCE Framework**: JUCE 7/8
- **IDE**: Visual Studio 2022 (`Builds/VisualStudio2022/MultibandCompressor.sln`)
- **Target Platforms**: Windows 64-bit (VST3, Standalone Plugin)
