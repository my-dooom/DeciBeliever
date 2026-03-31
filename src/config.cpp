#include "config.hpp"
#include <cstring>
#include <iostream>

namespace av {

void print_usage() {
  std::cout <<
      R"(Usage: DeciBeliever [options]

Modes:
  --wav-compare <ref.wav> <test.wav>   Compare two WAV files
  --wav-check   <file.wav>             Single-file integrity check
  --live        <device-index>         Live stream verification (PortAudio)

Options:
  --snr-threshold <dB>       Minimum acceptable SNR        (default: 30.0)
  --mse-threshold <val>      Maximum acceptable MSE         (default: 0.001)
  --window-ms     <ms>       Analysis window in ms          (default: 50)
  --overlap       <0-1>      Window overlap fraction         (default: 0.5)
  --effects                  Enable effect-aware analysis
  --json          <path>     Write JSON report to file
  -v, --verbose              Verbose diagnostics
  -h, --help                 Show this help
)";
}

static bool match(const char *arg, const char *flag) {
  return std::strcmp(arg, flag) == 0;
}

std::optional<Config> parse_args(int argc, char *argv[]) {
  Config cfg;

  for (int i = 1; i < argc; ++i) {
    const char *a = argv[i];

    if (match(a, "-h") || match(a, "--help")) {
      cfg.mode = Mode::Help;
      return cfg;
    } else if (match(a, "--wav-compare")) {
      if (i + 1 >= argc) {
        std::cerr << "Error: --wav-compare requires at least one file path\n";
        return std::nullopt;
      }

      cfg.mode = Mode::WavCompare;

      // One path provided: use generated sine as reference.
      if (i + 2 >= argc || argv[i + 2][0] == '-') {
        cfg.ref_path = "1";
        cfg.test_path = argv[++i];
      } else {
        cfg.ref_path = argv[++i];
        cfg.test_path = argv[++i];
      }
    }

    else if (match(a, "--wav-check")) {
      if (i + 1 >= argc) {
        std::cerr << "Error: --wav-check requires a file path\n";
        return std::nullopt;
      }
      cfg.mode = Mode::WavCheck;
      cfg.ref_path = argv[++i];
    } else if (match(a, "--live")) {
      if (i + 1 >= argc) {
        std::cerr << "Error: --live requires a device index\n";
        return std::nullopt;
      }
      cfg.mode = Mode::Live;
      cfg.device_index = std::stoi(argv[++i]);
    } else if (match(a, "--snr-threshold")) {
      if (i + 1 >= argc)
        return std::nullopt;
      cfg.snr_threshold = std::stod(argv[++i]);
    } else if (match(a, "--mse-threshold")) {
      if (i + 1 >= argc)
        return std::nullopt;
      cfg.mse_threshold = std::stod(argv[++i]);
    } else if (match(a, "--window-ms")) {
      if (i + 1 >= argc)
        return std::nullopt;
      cfg.window_ms = std::stod(argv[++i]);
    } else if (match(a, "--overlap")) {
      if (i + 1 >= argc)
        return std::nullopt;
      cfg.overlap = std::stod(argv[++i]);
    } else if (match(a, "--effects")) {
      cfg.effects_enabled = true;
    } else if (match(a, "--json")) {
      if (i + 1 >= argc)
        return std::nullopt;
      cfg.json_path = argv[++i];
    } else if (match(a, "-v") || match(a, "--verbose")) {
      cfg.verbose = true;
    } else {
      std::cerr << "Unknown argument: " << a << "\n";
      return std::nullopt;
    }
  }

  return cfg;
}

} // namespace av
