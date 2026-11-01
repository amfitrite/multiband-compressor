## 2024-05-24 - JUCE String Lookups in processAudio
**Learning:** Calling `apvts.getRawParameterValue("string")` inside the real-time audio callback (`processAudio`) or during frequent state updates causes severe UI lags and CPU spikes when loading parameters.
**Action:** Always cache pointers to `std::atomic<float>` in the constructor (using a `CachedParams` struct) and read from those pointers using `->load()` in the DSP loop.
