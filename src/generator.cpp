#include "generator.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

namespace av {


AudioBuffer make_buffer_sine(size_t frames, float freq, uint32_t sr) {
    constexpr float kPi = 3.14159265358979323846f;
    AudioBuffer buf;
    buf.sample_rate  = sr;
    buf.channels     = 1;
    buf.total_frames = static_cast<uint32_t>(frames);
    buf.samples.resize(frames);
    for (size_t i = 0; i < frames; ++i) {
        buf.samples[i] = std::sin(2.0f * kPi * freq * static_cast<float>(i) / static_cast<float>(sr));
    }
    return buf;
}

} // namespace av

