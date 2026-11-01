## 2024-11-01 - Real-Time Thread Safety and Performance
**Learning:** In JUCE audio development, performing dynamic string operations (like `prefix + "_gain"`) and map lookups (`apvts.getRawParameterValue()`) inside the per-sample or per-block `processAudio` callback causes massive CPU overhead and blocks the real-time audio thread, violating real-time safety principles.
**Action:** Always cache atomic parameter pointers at initialization (e.g., in the processor's constructor) and use the cached pointers inside the `processAudio` loop for zero-overhead, real-time safe parameter access.
