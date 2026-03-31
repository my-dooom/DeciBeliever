#include "report.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <nlohmann/json.hpp>

namespace av {

void report_console(const VerifyResult& result, const Config& cfg) {
    std::cout << "\n===== Audio Verifier Report =====\n\n";

    std::cout << "Result: " << (result.passed ? "PASS" : "FAIL") << "\n\n";

    if (cfg.mode == Mode::WavCompare) {
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "  SNR             : " << result.metrics.snr_db        << " dB\n";
        std::cout << "  MSE             : " << result.metrics.mse           << "\n";
        std::cout << "  Cross-correlation: " << result.metrics.cross_corr    << "\n";
        std::cout << "  Spectral distance: " << result.metrics.spectral_dist << "\n";
        std::cout << "  Dynamic range   : " << result.metrics.dynamic_range << " dB\n";
    }

    if (cfg.mode == Mode::WavCheck || cfg.mode == Mode::Live) {
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "  Dynamic range   : " << result.dynamic_range << " dB\n";
        std::cout << "  Clipping ratio  : " << result.clip_ratio * 100.0 << " %\n";
        std::cout << "  Silent          : " << (result.silent ? "Yes" : "No") << "\n";
    }

    if (cfg.effects_enabled) {
        std::cout << "\n  Effect analysis:\n";
        std::cout << "    THD           : " << result.effects.thd  << "\n";
        std::cout << "    RT60          : " << result.effects.rt60 << " s\n";
        std::cout << "    Distorted     : " << (result.effects.distorted ? "Yes" : "No") << "\n";
        std::cout << "    Reverbed      : " << (result.effects.reverbed  ? "Yes" : "No") << "\n";
    }

    if (!result.warnings.empty()) {
        std::cout << "\n  Warnings:\n";
        for (const auto& w : result.warnings)
            std::cout << "    [WARN] " << w << "\n";
    }

    if (!result.errors.empty()) {
        std::cout << "\n  Errors:\n";
        for (const auto& e : result.errors)
            std::cout << "    [ERR]  " << e << "\n";
    }

    std::cout << "\n=================================\n";
}

std::string report_json(const VerifyResult& result, const Config& cfg) {
    nlohmann::json j;
    j["passed"]       = result.passed;
    j["format_valid"] = result.format_valid;
    j["silent"]       = result.silent;
    j["clip_ratio"]   = result.clip_ratio;
    j["dynamic_range"] = result.dynamic_range;

    if (cfg.mode == Mode::WavCompare) {
        j["metrics"]["snr_db"]        = result.metrics.snr_db;
        j["metrics"]["mse"]           = result.metrics.mse;
        j["metrics"]["cross_corr"]    = result.metrics.cross_corr;
        j["metrics"]["spectral_dist"] = result.metrics.spectral_dist;
        j["metrics"]["dynamic_range"] = result.metrics.dynamic_range;
    }

    if (cfg.effects_enabled) {
        j["effects"]["thd"]       = result.effects.thd;
        j["effects"]["rt60"]      = result.effects.rt60;
        j["effects"]["distorted"] = result.effects.distorted;
        j["effects"]["reverbed"]  = result.effects.reverbed;
    }

    j["warnings"] = result.warnings;
    j["errors"]   = result.errors;

    return j.dump(2);
}

bool report_json_file(const VerifyResult& result, const Config& cfg,
                      const std::string& path) {
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        std::cerr << "Cannot write JSON report to: " << path << "\n";
        return false;
    }
    ofs << report_json(result, cfg);
    return true;
}

} // namespace av
