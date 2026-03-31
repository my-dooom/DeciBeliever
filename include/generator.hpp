#pragma once

#include <cstddef>
#include <cstdint>

#include "audio_io.hpp"

namespace av {

AudioBuffer make_buffer_sine(size_t frames, float freq, uint32_t sr = 44100);

} // namespace av
