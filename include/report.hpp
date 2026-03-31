#pragma once
#include "verifier.hpp"
#include "config.hpp"
#include <string>

namespace av {

// ---------- Reporting ----------

/// Print a human-readable report to stdout.
void report_console(const VerifyResult& result, const Config& cfg);

/// Serialise the result to a JSON string.
std::string report_json(const VerifyResult& result, const Config& cfg);

/// Write JSON report to a file. Returns true on success.
bool report_json_file(const VerifyResult& result, const Config& cfg,
                      const std::string& path);

} // namespace av
