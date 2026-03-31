# DeciBeliever

A modular C++17 tool for verifying audio signal integrity, computing quality metrics, and performing effect-aware analysis on WAV files and live audio streams.

---

## Features

| Category | Capability |
|---|---|
| **Input** | WAV files (via dr\_wav), live capture (via PortAudio, optional) |
| **Integrity** | Format validation, silence detection, clipping detection, corruption checks |
| **Metrics** | SNR, MSE, cross-correlation, spectral distance, dynamic range |
| **Effects** | Distortion detection (THD), reverb analysis (RT60 estimate) |
| **Reporting** | Console summary, optional JSON output |
| **Testing** | Unit tests (Catch2) for metrics + integration scenarios |

---

## Building

### Prerequisites

* CMake ≥ 3.16
* C++17 compiler (GCC 8+, Clang 7+, MSVC 2019+)
* (Optional) PortAudio for live capture

### Steps

```bash
# Clone and enter the project
cd DeciBeliever

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run tests
cd build && ctest --output-on-failure
```

---

## Usage

```
DeciBeliever [options]

Modes:
  --wav-compare <ref.wav> <test.wav>   Compare two WAV files
  --wav-check   <file.wav>             Single-file integrity check
  --live        <device-index>         Live stream verification (requires PortAudio)

Options:
  --snr-threshold <dB>       Minimum acceptable SNR (default: 30.0)
  --mse-threshold <val>      Maximum acceptable MSE  (default: 0.001)
  --window-ms     <ms>       Analysis window in ms   (default: 50)
  --overlap       <0-1>      Window overlap fraction  (default: 0.5)
  --effects                  Enable effect-aware analysis
  --json          <path>     Write JSON report to file
  -v, --verbose              Verbose diagnostics
  -h, --help                 Show this help
```

### Examples

```bash
# Compare reference and processed WAV
DeciBeliever --wav-compare ref.wav processed.wav --snr-threshold 40

# Single-file integrity check with JSON report
DeciBeliever --wav-check recording.wav --json report.json

# Effect-aware comparison
DeciBeliever --wav-compare dry.wav wet.wav --effects
```

---

## Metric Definitions

| Metric | Formula / Description |
|---|---|
| **SNR** | $10 \log_{10}\!\left(\frac{\sum x^2}{\sum (x - y)^2}\right)$ — Signal-to-noise ratio in dB |
| **MSE** | $\frac{1}{N}\sum_{i=0}^{N-1}(x_i - y_i)^2$ — Mean squared error |
| **Cross-correlation** | Normalised Pearson correlation of the two signals |
| **Spectral distance** | RMS of magnitude spectrum difference (via FFT) |
| **Dynamic range** | $20 \log_{10}\!\left(\frac{\max |x|}{\text{RMS}(x)}\right)$ in dB |
| **THD** | Total harmonic distortion — ratio of harmonic power to fundamental |
| **RT60 estimate** | Time for impulse response energy to decay by 60 dB |

---

## Project Structure

```
DeciBeliever/
├── CMakeLists.txt          Root build configuration
├── include/                Public headers
│   ├── config.hpp          CLI parsing & threshold config
│   ├── audio_io.hpp        WAV / live audio I/O
│   ├── metrics.hpp         Signal quality metrics
│   ├── effects.hpp         Effect-aware analysis
│   ├── verifier.hpp        Orchestration / verification logic
│   └── report.hpp          Console & JSON reporting
├── src/                    Implementation
├── tests/                  Catch2 unit & integration tests
├── assets/                 Test audio files / generation scripts
└── third_party/            Vendored dependencies (dr_wav, kissfft)
```

---

## License

MIT
