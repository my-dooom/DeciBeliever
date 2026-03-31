#pragma once
#include "config.hpp"
#include "audio_io.hpp"
#include "metrics.hpp"
#include "effects.hpp"
#include <string>
#include <vector>

namespace av {

// ---------- Verification orchestration ----------

struct VerifyResult {
    bool passed = true;

    // Integrity
    bool format_valid   = true;
    bool silent         = false;
    double clip_ratio   = 0.0;

    // Metrics (populated for compare mode)
    MetricResult metrics;

    // Single-file diagnostics
    double dynamic_range = 0.0;

    // Effect analysis (populated when effects enabled)
    EffectResult effects;

    // Human-readable diagnostics
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

/// Run verification according to the given Config.
VerifyResult verify(const Config& cfg);

/// Compare two loaded audio buffers.
VerifyResult verify_compare(const AudioBuffer& ref, const AudioBuffer& test,
                            const Config& cfg);

/// Single-file integrity check.
VerifyResult verify_single(const AudioBuffer& buf, const Config& cfg);

} // namespace av
