#define _USE_MATH_DEFINES
#include <cmath>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "metrics.hpp"
#include <vector>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

// ---- helpers: generate test signals ----

static std::vector<float> make_sine(size_t n, float freq, float sr, float amp = 1.0f) {
    std::vector<float> s(n);
    for (size_t i = 0; i < n; ++i) {
        s[i] = amp * std::sin(2.0f * static_cast<float>(M_PI) * freq * i / sr);
    }
    return s;
}

static std::vector<float> make_silence(size_t n) {
    return std::vector<float>(n, 0.0f);
}

static std::vector<float> add_noise(const std::vector<float>& sig, float level) {
    std::vector<float> out = sig;
    // deterministic "noise" for reproducibility
    for (size_t i = 0; i < out.size(); ++i) {
        float pseudo = level * (static_cast<float>((i * 73 + 17) % 1000) / 500.0f - 1.0f);
        out[i] += pseudo;
    }
    return out;
}

// ---- SNR ----

TEST_CASE("SNR of identical signals is very high", "[metrics][snr]") {
    auto sig = make_sine(1024, 440.0f, 44100.0f);
    double snr = av::compute_snr(sig.data(), sig.data(), sig.size());
    REQUIRE(snr >= 200.0);
}

TEST_CASE("SNR decreases with added noise", "[metrics][snr]") {
    auto ref  = make_sine(4096, 440.0f, 44100.0f);
    auto noisy = add_noise(ref, 0.01f);
    double snr = av::compute_snr(ref.data(), noisy.data(), ref.size());
    REQUIRE(snr > 0.0);
    REQUIRE(snr < 200.0);
}

// ---- MSE ----

TEST_CASE("MSE of identical signals is zero", "[metrics][mse]") {
    auto sig = make_sine(1024, 440.0f, 44100.0f);
    double mse = av::compute_mse(sig.data(), sig.data(), sig.size());
    REQUIRE_THAT(mse, WithinAbs(0.0, 1e-12));
}

TEST_CASE("MSE is positive for different signals", "[metrics][mse]") {
    auto ref   = make_sine(1024, 440.0f, 44100.0f);
    auto other = make_sine(1024, 880.0f, 44100.0f);
    double mse = av::compute_mse(ref.data(), other.data(), ref.size());
    REQUIRE(mse > 0.0);
}

// ---- Cross-correlation ----

TEST_CASE("Cross-correlation of identical signals is 1", "[metrics][corr]") {
    auto sig = make_sine(1024, 440.0f, 44100.0f);
    double cc = av::compute_cross_correlation(sig.data(), sig.data(), sig.size());
    REQUIRE_THAT(cc, WithinAbs(1.0, 1e-6));
}

TEST_CASE("Cross-correlation of negated signal is -1", "[metrics][corr]") {
    auto sig = make_sine(1024, 440.0f, 44100.0f);
    std::vector<float> neg(sig.size());
    for (size_t i = 0; i < sig.size(); ++i) neg[i] = -sig[i];
    double cc = av::compute_cross_correlation(sig.data(), neg.data(), sig.size());
    REQUIRE_THAT(cc, WithinAbs(-1.0, 1e-6));
}

// ---- Spectral distance ----

TEST_CASE("Spectral distance of identical signals is zero", "[metrics][spectral]") {
    auto sig = make_sine(1024, 440.0f, 44100.0f);
    double sd = av::compute_spectral_distance(sig.data(), sig.data(), sig.size());
    REQUIRE_THAT(sd, WithinAbs(0.0, 1e-6));
}

TEST_CASE("Spectral distance of different frequencies is positive", "[metrics][spectral]") {
    auto a = make_sine(1024, 440.0f, 44100.0f);
    auto b = make_sine(1024, 880.0f, 44100.0f);
    double sd = av::compute_spectral_distance(a.data(), b.data(), a.size());
    REQUIRE(sd > 0.0);
}

// ---- Dynamic range ----

TEST_CASE("Dynamic range of a sine wave", "[metrics][dr]") {
    auto sig = make_sine(4096, 440.0f, 44100.0f);
    double dr = av::compute_dynamic_range(sig.data(), sig.size());
    // A pure sine has crest factor ~3.01 dB
    REQUIRE_THAT(dr, WithinAbs(3.01, 0.1));
}

// ---- Silence detection ----

TEST_CASE("Silence detected on zero buffer", "[metrics][silence]") {
    auto s = make_silence(1024);
    REQUIRE(av::is_silent(s.data(), s.size()));
}

TEST_CASE("Non-silent signal not detected as silent", "[metrics][silence]") {
    auto s = make_sine(1024, 440.0f, 44100.0f);
    REQUIRE_FALSE(av::is_silent(s.data(), s.size()));
}

// ---- Clipping ----

TEST_CASE("No clipping for normalised sine", "[metrics][clipping]") {
    auto s = make_sine(1024, 440.0f, 44100.0f, 0.9f);
    double cr = av::clipping_ratio(s.data(), s.size());
    REQUIRE_THAT(cr, WithinAbs(0.0, 1e-9));
}

TEST_CASE("Clipping detected for over-driven signal", "[metrics][clipping]") {
    auto s = make_sine(1024, 440.0f, 44100.0f, 2.0f);
    // Clamp to produce clipping
    for (auto& v : s) {
        if (v > 1.0f)  v = 1.0f;
        if (v < -1.0f) v = -1.0f;
    }
    double cr = av::clipping_ratio(s.data(), s.size());
    REQUIRE(cr > 0.0);
}

// ---- Edge cases ----

TEST_CASE("Zero-length inputs return safe defaults", "[metrics][edge]") {
    REQUIRE(av::compute_snr(nullptr, nullptr, 0) == 0.0);
    REQUIRE(av::compute_mse(nullptr, nullptr, 0) == 0.0);
    REQUIRE(av::compute_cross_correlation(nullptr, nullptr, 0) == 0.0);
    REQUIRE(av::compute_spectral_distance(nullptr, nullptr, 0) == 0.0);
    REQUIRE(av::compute_dynamic_range(nullptr, 0) == 0.0);
}
