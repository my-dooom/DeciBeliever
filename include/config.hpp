#pragma once
#include <optional>
#include <string>

namespace av {

// ---------- CLI / threshold configuration ----------

enum class Mode { WavCompare, WavCheck, Live, Help };

struct Config {
  Mode mode = Mode::Help;

  // File paths
  std::string ref_path;
  std::string test_path;

  // Live capture
  int device_index = -1;

  // Thresholds
  double snr_threshold = 30.0; // dB
  double mse_threshold = 0.001;
  double corr_threshold = 0.95;

  // Windowing
  double window_ms = 50.0;
  double overlap = 0.5; // 0–1

  // Flags
  bool effects_enabled = false;
  bool verbose = false;
  std::optional<std::string> json_path;
};

/// Parse command-line arguments into a Config. Returns std::nullopt on error.
std::optional<Config> parse_args(int argc, char *argv[]);

/// Print usage / help text to stdout.
void print_usage();

} // namespace av
