#pragma once
#include <cstddef>

namespace av {

// ---------- Signal quality metrics ----------

struct MetricResult {
    double snr_db = 0.0;        // Signal-to-noise ratio (dB)
    double mse = 0.0;           // Mean squared error
    double cross_corr = 0.0;    // Normalised cross-correlation
    double spectral_dist = 0.0; // RMS spectral magnitude difference
    double dynamic_range = 0.0; // Peak-to-RMS ratio (dB)
};

/// Compute SNR between reference and test signals (dB).
double compute_snr(const float* ref, const float* test, size_t n);

/// Compute mean squared error between reference and test signals.
double compute_mse(const float* ref, const float* test, size_t n);

/// Compute normalised cross-correlation between reference and test signals.
double compute_cross_correlation(const float* ref, const float* test, size_t n);

/// Compute RMS spectral magnitude distance between reference and test signals.
double compute_spectral_distance(const float* ref, const float* test, size_t n);

/// Compute dynamic range (peak-to-RMS ratio in dB) of a signal.
double compute_dynamic_range(const float* signal, size_t n);

/// Compute all metrics at once.
MetricResult compute_all_metrics(const float* ref, const float* test, size_t n);

/// Returns true if the signal is effectively silent (below threshold).
bool is_silent(const float* signal, size_t n, float threshold = 1e-5f);

/// Returns the ratio of clipped samples (|sample| >= 1.0).
double clipping_ratio(const float* signal, size_t n);

} // namespace av

