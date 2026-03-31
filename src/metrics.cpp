#include "metrics.hpp"

#include "kiss_fft.h"
#include "kiss_fftr.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace av {

// -------- helpers --------

static double rms(const float *s, size_t n) {
  double sum = 0.0;
  for (size_t i = 0; i < n; ++i)
    sum += static_cast<double>(s[i]) * s[i];
  return std::sqrt(sum / static_cast<double>(n));
}

static double peak(const float *s, size_t n) {
  double mx = 0.0;
  for (size_t i = 0; i < n; ++i)
    mx = std::max(mx, std::abs(static_cast<double>(s[i])));
  return mx;
}

// -------- public API --------

double compute_snr(const float *ref, const float *test, size_t n) {
  if (n == 0)
    return 0.0;

  double signal_power = 0.0;
  double noise_power = 0.0;
  for (size_t i = 0; i < n; ++i) {
    double r = ref[i];
    double t = test[i];
    signal_power += r * r;
    noise_power += (r - t) * (r - t);
  }

  if (noise_power < 1e-30)
    return 300.0; // effectively identical
  return 10.0 * std::log10(signal_power / noise_power);
}

double compute_mse(const float *ref, const float *test, size_t n) {
  if (n == 0)
    return 0.0;

  double sum = 0.0;
  for (size_t i = 0; i < n; ++i) {
    double d = static_cast<double>(ref[i]) - test[i];
    sum += d * d;
  }
  return sum / static_cast<double>(n);
}

double compute_cross_correlation(const float *ref, const float *test,
                                 size_t n) {
  if (n == 0)
    return 0.0;

  double mean_r = 0.0, mean_t = 0.0;
  for (size_t i = 0; i < n; ++i) {
    mean_r += ref[i];
    mean_t += test[i];
  }
  mean_r /= n;
  mean_t /= n;

  double num = 0.0, den_r = 0.0, den_t = 0.0;
  for (size_t i = 0; i < n; ++i) {
    double dr = ref[i] - mean_r;
    double dt = test[i] - mean_t;
    num += dr * dt;
    den_r += dr * dr;
    den_t += dt * dt;
  }

  double denom = std::sqrt(den_r * den_t);
  if (denom < 1e-30)
    return 0.0;
  return num / denom;
}

double compute_spectral_distance(const float *ref, const float *test,
                                 size_t n) {
  if (n == 0)
    return 0.0;

  // Round up to next power of 2 for FFT
  size_t nfft = 1;
  while (nfft < n)
    nfft <<= 1;

  kiss_fftr_cfg cfg =
      kiss_fftr_alloc(static_cast<int>(nfft), 0, nullptr, nullptr);
  if (!cfg)
    return 0.0;

  std::vector<float> buf_r(nfft, 0.0f);
  std::vector<float> buf_t(nfft, 0.0f);
  std::copy(ref, ref + n, buf_r.begin());
  std::copy(test, test + n, buf_t.begin());

  size_t nbins = nfft / 2 + 1;
  std::vector<kiss_fft_cpx> spec_r(nbins);
  std::vector<kiss_fft_cpx> spec_t(nbins);

  kiss_fftr(cfg, buf_r.data(), spec_r.data());
  kiss_fftr(cfg, buf_t.data(), spec_t.data());

  double sum_sq = 0.0;
  for (size_t i = 0; i < nbins; ++i) {
    double mag_r =
        std::sqrt(spec_r[i].r * spec_r[i].r + spec_r[i].i * spec_r[i].i);
    double mag_t =
        std::sqrt(spec_t[i].r * spec_t[i].r + spec_t[i].i * spec_t[i].i);
    double diff = mag_r - mag_t;
    sum_sq += diff * diff;
  }

  kiss_fftr_free(cfg);
  return std::sqrt(sum_sq / static_cast<double>(nbins));
}

double compute_dynamic_range(const float *signal, size_t n) {
  if (n == 0)
    return 0.0;

  double pk = peak(signal, n);
  double r = rms(signal, n);
  if (r < 1e-30)
    return 0.0;

  return 20.0 * std::log10(pk / r);
}

MetricResult compute_all_metrics(const float *ref, const float *test,
                                 size_t n) {
  MetricResult m;
  m.snr_db = compute_snr(ref, test, n);
  m.mse = compute_mse(ref, test, n);
  m.cross_corr = compute_cross_correlation(ref, test, n);
  m.spectral_dist = compute_spectral_distance(ref, test, n);
  m.dynamic_range = compute_dynamic_range(ref, n);
  return m;
}

// -------- integrity --------

bool is_silent(const float *signal, size_t n, float threshold) {
  for (size_t i = 0; i < n; ++i) {
    if (std::abs(signal[i]) > threshold)
      return false;
  }
  return true;
}

double clipping_ratio(const float *signal, size_t n) {
  if (n == 0)
    return 0.0;
  size_t clipped = 0;
  for (size_t i = 0; i < n; ++i) {
    if (std::abs(signal[i]) >= 1.0f)
      ++clipped;
  }
  return static_cast<double>(clipped) / static_cast<double>(n);
}

} // namespace av
