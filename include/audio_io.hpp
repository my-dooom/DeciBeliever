#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace av {

// ---------- Audio buffer ----------

struct AudioBuffer {
    std::vector<float> samples;   // interleaved PCM, normalised to [-1, 1]
    uint32_t sample_rate = 0;
    uint16_t channels    = 0;
    uint32_t total_frames = 0;    // frames = samples.size() / channels
};

// ---------- WAV I/O ----------

/// Load a WAV file into an AudioBuffer. Returns std::nullopt on failure.
std::optional<AudioBuffer> load_wav(const std::string& path);

/// Validate WAV header without loading full PCM data.
/// Returns an error string on failure, or std::nullopt if valid.
std::optional<std::string> validate_wav_header(const std::string& path);

// ---------- Live capture (requires PortAudio) ----------

#ifdef HAS_PORTAUDIO

/// List available audio input devices (index, name).
std::vector<std::pair<int, std::string>> list_input_devices();

/// Capture `duration_sec` seconds from device `device_index`.
std::optional<AudioBuffer> capture_live(int device_index,
                                        double duration_sec,
                                        uint32_t sample_rate = 44100,
                                        uint16_t channels = 1);

#endif // HAS_PORTAUDIO

} // namespace av
