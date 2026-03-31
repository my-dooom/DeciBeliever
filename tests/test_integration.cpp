#define _USE_MATH_DEFINES
#include <cmath>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "config.hpp"
#include "verifier.hpp"
#include "report.hpp"
#include "audio_io.hpp"
#include <vector>
#include <cstring>

using Catch::Matchers::WithinAbs;

// ---- helper: build an AudioBuffer in memory ----

static av::AudioBuffer make_buffer_sine(size_t frames, float freq, uint32_t sr = 44100) {
    av::AudioBuffer buf;
    buf.sample_rate  = sr;
    buf.channels     = 1;
    buf.total_frames = static_cast<uint32_t>(frames);
    buf.samples.resize(frames);
    for (size_t i = 0; i < frames; ++i) {
        buf.samples[i] = std::sin(2.0f * static_cast<float>(M_PI) * freq * i / sr);
    }
    return buf;
}

static av::AudioBuffer make_buffer_silent(size_t frames, uint32_t sr = 44100) {
    av::AudioBuffer buf;
    buf.sample_rate  = sr;
    buf.channels     = 1;
    buf.total_frames = static_cast<uint32_t>(frames);
    buf.samples.assign(frames, 0.0f);
    return buf;
}

// ---- CLI parsing ----

TEST_CASE("Parse --wav-compare args", "[config]") {
    const char* args[] = {"app", "--wav-compare", "ref.wav", "test.wav"};
    auto cfg = av::parse_args(4, const_cast<char**>(args));
    REQUIRE(cfg.has_value());
    REQUIRE(cfg->mode == av::Mode::WavCompare);
    REQUIRE(cfg->ref_path == "ref.wav");
    REQUIRE(cfg->test_path == "test.wav");
}

TEST_CASE("Parse --wav-check args", "[config]") {
    const char* args[] = {"app", "--wav-check", "file.wav"};
    auto cfg = av::parse_args(3, const_cast<char**>(args));
    REQUIRE(cfg.has_value());
    REQUIRE(cfg->mode == av::Mode::WavCheck);
    REQUIRE(cfg->ref_path == "file.wav");
}

TEST_CASE("Parse --help produces Help mode", "[config]") {
    const char* args[] = {"app", "--help"};
    auto cfg = av::parse_args(2, const_cast<char**>(args));
    REQUIRE(cfg.has_value());
    REQUIRE(cfg->mode == av::Mode::Help);
}

TEST_CASE("Unknown args return nullopt", "[config]") {
    const char* args[] = {"app", "--bogus"};
    auto cfg = av::parse_args(2, const_cast<char**>(args));
    REQUIRE_FALSE(cfg.has_value());
}

TEST_CASE("Threshold options are parsed", "[config]") {
    const char* args[] = {"app", "--wav-check", "f.wav",
                          "--snr-threshold", "50",
                          "--mse-threshold", "0.01",
                          "--effects", "-v"};
    auto cfg = av::parse_args(9, const_cast<char**>(args));
    REQUIRE(cfg.has_value());
    REQUIRE_THAT(cfg->snr_threshold, WithinAbs(50.0, 1e-9));
    REQUIRE_THAT(cfg->mse_threshold, WithinAbs(0.01, 1e-9));
    REQUIRE(cfg->effects_enabled);
    REQUIRE(cfg->verbose);
}

// ---- verify_compare in-memory ----

TEST_CASE("Identical buffers pass verification", "[verifier]") {
    auto buf = make_buffer_sine(4096, 440.0f);
    av::Config cfg;
    cfg.mode = av::Mode::WavCompare;
    auto res = av::verify_compare(buf, buf, cfg);
    REQUIRE(res.passed);
    REQUIRE(res.metrics.snr_db >= 200.0);
    REQUIRE_THAT(res.metrics.mse, WithinAbs(0.0, 1e-9));
}

TEST_CASE("Sample-rate mismatch produces error", "[verifier]") {
    auto a = make_buffer_sine(4096, 440.0f, 44100);
    auto b = make_buffer_sine(4096, 440.0f, 48000);
    av::Config cfg;
    cfg.mode = av::Mode::WavCompare;
    auto res = av::verify_compare(a, b, cfg);
    REQUIRE_FALSE(res.passed);
    REQUIRE_FALSE(res.errors.empty());
}

TEST_CASE("Silent test signal produces warning", "[verifier]") {
    auto ref  = make_buffer_sine(4096, 440.0f);
    auto test = make_buffer_silent(4096);
    av::Config cfg;
    cfg.mode = av::Mode::WavCompare;
    cfg.snr_threshold = -999; // don't fail on SNR
    cfg.mse_threshold = 999;
    auto res = av::verify_compare(ref, test, cfg);
    bool has_silent_warning = false;
    for (const auto& w : res.warnings) {
        if (w.find("silent") != std::string::npos ||
            w.find("Silent") != std::string::npos) {
            has_silent_warning = true;
        }
    }
    REQUIRE(has_silent_warning);
}

// ---- verify_single ----

TEST_CASE("Single-file check on silent buffer", "[verifier]") {
    auto buf = make_buffer_silent(4096);
    av::Config cfg;
    cfg.mode = av::Mode::WavCheck;
    auto res = av::verify_single(buf, cfg);
    REQUIRE(res.silent);
}

// ---- JSON report ----

TEST_CASE("JSON report contains expected keys", "[report]") {
    auto buf = make_buffer_sine(4096, 440.0f);
    av::Config cfg;
    cfg.mode = av::Mode::WavCompare;
    auto res = av::verify_compare(buf, buf, cfg);
    std::string json = av::report_json(res, cfg);
    REQUIRE(json.find("\"passed\"") != std::string::npos);
    REQUIRE(json.find("\"metrics\"") != std::string::npos);
    REQUIRE(json.find("\"snr_db\"") != std::string::npos);
}
