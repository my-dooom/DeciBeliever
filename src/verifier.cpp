#include "verifier.hpp"
#include "generator.hpp"

namespace av {

VerifyResult verify_single(const AudioBuffer &buf, const Config &cfg) {
  VerifyResult res;

  size_t n = buf.samples.size();
  const float *data = buf.samples.data();

  // Silence check
  res.silent = is_silent(data, n);
  if (res.silent) {
    res.warnings.push_back("Signal is silent");
  }

  // Clipping
  res.clip_ratio = clipping_ratio(data, n);
  if (res.clip_ratio > 0.01) {
    res.warnings.push_back(
        "Clipping detected: " + std::to_string(res.clip_ratio * 100.0) +
        "% of samples");
  }



  // Dynamic range
  res.dynamic_range = compute_dynamic_range(data, n);

  // Effect-aware analysis
  if (cfg.effects_enabled) {
    res.effects = analyse_effects(data, n, buf.sample_rate);
    if (res.effects.distorted) {
      res.warnings.push_back("High harmonic distortion detected (THD=" +
                             std::to_string(res.effects.thd) + ")");
    }
    if (res.effects.reverbed) {
      res.warnings.push_back("Long reverb tail detected (RT60=" +
                             std::to_string(res.effects.rt60) + "s)");
    }
  }

  res.passed = res.errors.empty();
  return res;
}

VerifyResult verify_compare(const AudioBuffer &ref, const AudioBuffer &test,
                            const Config &cfg) {
  VerifyResult res;

  if (cfg.verbose) {
    const size_t test_samples = test.samples.size();
    const size_t test_frames = (test.channels == 0)
                                   ? 0
                                   : (test_samples / test.channels);
    res.warnings.push_back("Test signal loaded: " +
                           std::to_string(test_samples) +
                           " samples (" + std::to_string(test_frames) +
                           " frames, " +
                           std::to_string(static_cast<unsigned>(test.channels)) +
                           " ch @ " + std::to_string(test.sample_rate) +
                           " Hz)");
  }

  // Format compatibility check
  if (ref.sample_rate != test.sample_rate) {
    res.errors.push_back(
        "Sample rate mismatch: " + std::to_string(ref.sample_rate) + " vs " +
        std::to_string(test.sample_rate));
  }
  if (ref.channels != test.channels) {
    res.errors.push_back(
        "Channel count mismatch: " + std::to_string(ref.channels) + " vs " +
        std::to_string(test.channels));
  }
  if (!res.errors.empty()) {
    res.passed = false;
    return res;
  }

  size_t n = std::min(ref.samples.size(), test.samples.size());
  if (ref.samples.size() != test.samples.size()) {
    res.warnings.push_back("Sample count mismatch – using shorter length (" +
                           std::to_string(n) + " samples)");
  }

  // Integrity on both signals
  if (is_silent(ref.samples.data(), n)) {
    res.warnings.push_back("Reference signal is silent");
  }
  if (is_silent(test.samples.data(), n)) {
    res.warnings.push_back("Test signal is silent");
  }

  res.clip_ratio = clipping_ratio(test.samples.data(), n);
  if (res.clip_ratio > 0.01) {
    res.warnings.push_back("Test signal clipping: " +
                           std::to_string(res.clip_ratio * 100.0) + "%");
  }

  // Compute metrics
  res.metrics = compute_all_metrics(ref.samples.data(), test.samples.data(), n);

  // Threshold checks
  if (res.metrics.snr_db < cfg.snr_threshold) {
    res.errors.push_back(
        "SNR below threshold: " + std::to_string(res.metrics.snr_db) +
        " dB < " + std::to_string(cfg.snr_threshold) + " dB");
  }
  if (res.metrics.mse > cfg.mse_threshold) {
    res.errors.push_back(
        "MSE above threshold: " + std::to_string(res.metrics.mse) + " > " +
        std::to_string(cfg.mse_threshold));
  }
  if (res.metrics.cross_corr < cfg.corr_threshold) {
    res.warnings.push_back("Cross-correlation low: " +
                           std::to_string(res.metrics.cross_corr));
  }

  // Dynamic range
  res.dynamic_range = compute_dynamic_range(ref.samples.data(), n);

  // Effect-aware
  if (cfg.effects_enabled) {
    res.effects = analyse_effects(test.samples.data(), n, test.sample_rate);
    if (res.effects.distorted)
      res.warnings.push_back("Distortion detected in test signal");
    if (res.effects.reverbed)
      res.warnings.push_back("Reverb detected in test signal");
  }

  res.passed = res.errors.empty();
  return res;
}
VerifyResult verify_compare_to_perfect_file(const Config &cfg);

VerifyResult verify(const Config &cfg) {
  VerifyResult res;

  switch (cfg.mode) {
  case Mode::WavCompare: {
    std::optional<AudioBuffer> ref, test;
    if (auto err = validate_wav_header(cfg.test_path)) {
      res.errors.push_back("Test file: " + *err);
      res.format_valid = false;
      res.passed = false;
      return res;
    }
    test = load_wav(cfg.test_path);
    if (cfg.ref_path == "1") {
      ref = make_buffer_sine(test->samples.size(), 440.0f, test->sample_rate);
    } else {
      if (auto err = validate_wav_header(cfg.ref_path)) {
        res.errors.push_back("Reference file: " + *err);
        res.format_valid = false;
        res.passed = false;
        return res;
      }
      ref = load_wav(cfg.ref_path);
    }


    // goto jumps to here if we're comparing against a built-in perfect sine wave
    perfect_file:
    
    if (!ref || !test) {
      res.errors.push_back("Failed to load WAV file(s)");
      res.passed = false;
      return res;
    }

    return verify_compare(*ref, *test, cfg);
  }

  case Mode::WavCheck: {
    if (auto err = validate_wav_header(cfg.ref_path)) {
      res.errors.push_back(*err);
      res.format_valid = false;
      res.passed = false;
      return res;
    }

    auto buf = load_wav(cfg.ref_path);
    if (!buf) {
      res.errors.push_back("Failed to load WAV file");
      res.passed = false;
      return res;
    }

    return verify_single(*buf, cfg);
  }

  case Mode::Live: {
#ifdef HAS_PORTAUDIO
    auto buf = capture_live(cfg.device_index, 5.0);
    if (!buf) {
      res.errors.push_back("Live capture failed");
      res.passed = false;
      return res;
    }
    return verify_single(*buf, cfg);
#else
    res.errors.push_back("Live capture requires PortAudio (not available)");
    res.passed = false;
    return res;
#endif
  }

  case Mode::Help:
  default:
    return res;
  }
}

} // namespace av
