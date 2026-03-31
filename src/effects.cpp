#include "effects.hpp"

#include "kiss_fft.h"
#include "kiss_fftr.h"

#include <cmath>
#include <vector>
#include <algorithm>

namespace av {

static bool is_impulse_like(const float* signal, size_t n) {
    if (!signal || n < 16) return false;

    size_t peak_idx = 0;
    double peak_abs = 0.0;
    double total_energy = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double v = static_cast<double>(signal[i]);
        const double a = std::abs(v);
        total_energy += v * v;
        if (a > peak_abs) {
            peak_abs = a;
            peak_idx = i;
        }
    }

    if (peak_abs < 1e-6 || total_energy < 1e-12) return false;

    const size_t early = std::max<size_t>(1, n / 10);
    double early_energy = 0.0;
    for (size_t i = 0; i < early; ++i) {
        const double v = static_cast<double>(signal[i]);
        early_energy += v * v;
    }

    double tail_energy = 0.0;
    for (size_t i = n - early; i < n; ++i) {
        const double v = static_cast<double>(signal[i]);
        tail_energy += v * v;
    }

    const double early_ratio = early_energy / total_energy;

    // RT60 from Schroeder integration is meaningful for impulse responses,
    // not steady-state program audio.
    const bool peak_is_early = peak_idx < early;
    const bool energy_front_loaded = early_ratio > 0.25;
    const bool decays_over_time = tail_energy < (0.25 * early_energy);
    return peak_is_early && energy_front_loaded && decays_over_time;
}

double compute_thd(const float* signal, size_t n,
                   uint32_t sample_rate, double fundamental_hz) {
    if (n == 0 || sample_rate == 0 || fundamental_hz <= 0.0) return 0.0;

    // FFT
    size_t nfft = 1;
    while (nfft < n) nfft <<= 1;

    kiss_fftr_cfg cfg = kiss_fftr_alloc(static_cast<int>(nfft), 0, nullptr, nullptr);
    if (!cfg) return 0.0;

    std::vector<float> buf(nfft, 0.0f);
    std::copy(signal, signal + n, buf.begin());

    size_t nbins = nfft / 2 + 1;
    std::vector<kiss_fft_cpx> spec(nbins);
    kiss_fftr(cfg, buf.data(), spec.data());
    kiss_fftr_free(cfg);

    // Compute magnitude spectrum
    std::vector<double> mag(nbins);
    for (size_t i = 0; i < nbins; ++i) {
        mag[i] = std::sqrt(spec[i].r * spec[i].r + spec[i].i * spec[i].i);
    }

    double bin_hz = static_cast<double>(sample_rate) / static_cast<double>(nfft);

    // Find fundamental bin
    size_t fund_bin = static_cast<size_t>(std::round(fundamental_hz / bin_hz));
    if (fund_bin >= nbins) return 0.0;

    double fund_power = mag[fund_bin] * mag[fund_bin];

    // Sum harmonic power (2nd through 10th)
    double harmonic_power = 0.0;
    for (int h = 2; h <= 10; ++h) {
        size_t hbin = fund_bin * h;
        if (hbin >= nbins) break;
        harmonic_power += mag[hbin] * mag[hbin];
    }

    if (fund_power < 1e-30) return 0.0;
    return std::sqrt(harmonic_power / fund_power);
}

double estimate_rt60(const float* impulse_response, size_t n,
                     uint32_t sample_rate) {
    if (n == 0 || sample_rate == 0) return 0.0;
    if (!is_impulse_like(impulse_response, n)) return 0.0;

    // Compute energy decay curve (Schroeder integration — reverse cumulative sum)
    std::vector<double> energy(n);
    for (size_t i = 0; i < n; ++i) {
        energy[i] = static_cast<double>(impulse_response[i]) * impulse_response[i];
    }

    // Reverse cumulative sum
    std::vector<double> edc(n);
    edc[n - 1] = energy[n - 1];
    for (size_t i = n - 1; i > 0; --i) {
        edc[i - 1] = edc[i] + energy[i - 1];
    }

    // Normalise to dB
    double max_edc = edc[0];
    if (max_edc < 1e-30) return 0.0;

    for (size_t i = 0; i < n; ++i) {
        edc[i] = 10.0 * std::log10(std::max(edc[i] / max_edc, 1e-30));
    }

    // Find the sample where EDC drops below -60 dB
    for (size_t i = 0; i < n; ++i) {
        if (edc[i] <= -60.0) {
            return static_cast<double>(i) / static_cast<double>(sample_rate);
        }
    }

    // If we never reach -60 dB, extrapolate from -5 to -25 dB range
    size_t i5  = 0;
    size_t i25 = 0;
    bool found5 = false, found25 = false;
    for (size_t i = 0; i < n; ++i) {
        if (!found5 && edc[i] <= -5.0)  { i5  = i; found5  = true; }
        if (!found25 && edc[i] <= -25.0) { i25 = i; found25 = true; break; }
    }

    if (found5 && found25 && i25 > i5) {
        double slope = (-25.0 - (-5.0)) / static_cast<double>(i25 - i5);
        double samples_60 = -60.0 / slope;
        return samples_60 / static_cast<double>(sample_rate);
    }

    return 0.0;
}

EffectResult analyse_effects(const float* signal, size_t n,
                             uint32_t sample_rate,
                             double fundamental_hz,
                             double thd_threshold,
                             double rt60_threshold) {
    EffectResult result;

    if (fundamental_hz > 0.0) {
        result.thd = compute_thd(signal, n, sample_rate, fundamental_hz);
    }

    result.rt60 = estimate_rt60(signal, n, sample_rate);

    result.distorted = result.thd  > thd_threshold;
    result.reverbed  = result.rt60 > rt60_threshold;

    return result;
}

} // namespace av
