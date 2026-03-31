#pragma once
#include <cstddef>
#include <cstdint>

namespace av {

// ---------- Effect-aware analysis ----------

struct EffectResult {
    double thd       = 0.0;   // Total harmonic distortion (ratio)
    double rt60      = 0.0;   // Estimated RT60 in seconds (0 if not applicable)
    bool   distorted = false; // True if THD exceeds threshold
    bool   reverbed  = false; // True if RT60 exceeds threshold
};

/// Compute total harmonic distortion (THD) of a signal.
/// `fundamental_hz` is the expected/dominant frequency.
double compute_thd(const float* signal, size_t n,
                   uint32_t sample_rate, double fundamental_hz);

/// Estimate RT60 (reverb decay time) from an impulse response.
double estimate_rt60(const float* impulse_response, size_t n,
                     uint32_t sample_rate);

/// Run full effect-aware analysis.
/// `thd_threshold` and `rt60_threshold` determine pass/fail flags.
EffectResult analyse_effects(const float* signal, size_t n,
                             uint32_t sample_rate,
                             double fundamental_hz = 0.0,
                             double thd_threshold  = 0.05,
                             double rt60_threshold  = 2.0);

} // namespace av
