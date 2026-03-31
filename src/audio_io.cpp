#include "audio_io.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <iostream>

namespace av {

std::optional<AudioBuffer> load_wav(const std::string& path) {
    drwav wav;
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        std::cerr << "Failed to open WAV: " << path << "\n";
        return std::nullopt;
    }

    AudioBuffer buf;
    buf.sample_rate  = wav.sampleRate;
    buf.channels     = static_cast<uint16_t>(wav.channels);
    buf.total_frames = static_cast<uint32_t>(wav.totalPCMFrameCount);

    size_t total_samples = wav.totalPCMFrameCount * wav.channels;
    buf.samples.resize(total_samples);

    drwav_uint64 frames_read = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount,
                                                          buf.samples.data());
    drwav_uninit(&wav);

    if (frames_read != wav.totalPCMFrameCount) {
        // Partial reads are not considered invalid, but we resize to actual.
        buf.total_frames = static_cast<uint32_t>(frames_read);
        buf.samples.resize(frames_read * buf.channels);
    }

    return buf;
}

std::optional<std::string> validate_wav_header(const std::string& path) {
    drwav wav;
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        return "Cannot open file or invalid WAV header";
    }

    if (wav.channels == 0) {
        drwav_uninit(&wav);
        return "WAV has 0 channels";
    }
    if (wav.sampleRate == 0) {
        drwav_uninit(&wav);
        return "WAV has 0 sample rate";
    }
    if (wav.totalPCMFrameCount == 0) {
        drwav_uninit(&wav);
        return "WAV has 0 frames";
    }

    drwav_uninit(&wav);
    return std::nullopt; // valid
}

// ---------- PortAudio live capture ----------
#ifdef HAS_PORTAUDIO
#include <portaudio.h>

std::vector<std::pair<int, std::string>> list_input_devices() {
    std::vector<std::pair<int, std::string>> devices;
    Pa_Initialize();

    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0) {
            devices.emplace_back(i, info->name);
        }
    }

    Pa_Terminate();
    return devices;
}

std::optional<AudioBuffer> capture_live(int device_index,
                                        double duration_sec,
                                        uint32_t sample_rate,
                                        uint16_t channels) {
    Pa_Initialize();

    PaStreamParameters params{};
    params.device           = device_index;
    params.channelCount     = channels;
    params.sampleFormat     = paFloat32;
    params.suggestedLatency = Pa_GetDeviceInfo(device_index)->defaultLowInputLatency;

    PaStream* stream = nullptr;
    PaError err = Pa_OpenStream(&stream, &params, nullptr,
                                sample_rate, 256, paClipOff, nullptr, nullptr);
    if (err != paNoError) {
        std::cerr << "PortAudio open error: " << Pa_GetErrorText(err) << "\n";
        Pa_Terminate();
        return std::nullopt;
    }

    Pa_StartStream(stream);

    size_t total_frames = static_cast<size_t>(duration_sec * sample_rate);
    AudioBuffer buf;
    buf.sample_rate  = sample_rate;
    buf.channels     = channels;
    buf.total_frames = static_cast<uint32_t>(total_frames);
    buf.samples.resize(total_frames * channels);

    size_t offset = 0;
    size_t chunk  = 256;
    while (offset < total_frames) {
        size_t frames_to_read = std::min(chunk, total_frames - offset);
        Pa_ReadStream(stream, buf.samples.data() + offset * channels,
                      static_cast<unsigned long>(frames_to_read));
        offset += frames_to_read;
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    return buf;
}

#endif // HAS_PORTAUDIO

} // namespace av
