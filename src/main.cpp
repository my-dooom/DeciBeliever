#include "config.hpp"
#include "report.hpp"
#include "verifier.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
  auto cfg = av::parse_args(argc, argv);
  if (!cfg) {
    av::print_usage();
    return EXIT_FAILURE;
  }

  if (cfg->mode == av::Mode::Help) {
    av::print_usage();
    return EXIT_SUCCESS;
  }

  if (cfg->verbose) {
    std::cout << "[verbose] Mode: ";
    switch (cfg->mode) {
    case av::Mode::WavCompare:
      std::cout << "WAV Compare\n";
      break;
    case av::Mode::WavCheck:
      std::cout << "WAV Check\n";
      break;
    case av::Mode::Live:
      std::cout << "Live Capture\n";
      break;
    default:
      break;
    }
  }

  av::VerifyResult result = av::verify(*cfg);

  av::report_console(result, *cfg);

  if (cfg->json_path) {
    if (av::report_json_file(result, *cfg, *cfg->json_path)) {
      std::cout << "JSON report written to: " << *cfg->json_path << "\n";
    }
  }

  return result.passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
