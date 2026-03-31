#!/usr/bin/env python3
"""Generate test WAV files for DeciBeliever.

Requires: pip install numpy soundfile
"""

import numpy as np
import os

try:
    import soundfile as sf
except ImportError:
    print("Install soundfile: pip install soundfile")
    raise

SR = 44100
DURATION = 1.0  # seconds
N = int(SR * DURATION)
t = np.linspace(0, DURATION, N, endpoint=False).astype(np.float32)

OUT_DIR = os.path.dirname(os.path.abspath(__file__))


def save(name: str, data: np.ndarray) -> None:
    path = os.path.join(OUT_DIR, name)
    sf.write(path, data, SR, subtype="FLOAT")
    print(f"  {name} ({len(data)} samples)")


print("Generating test WAV files...")

# 1. Clean 440 Hz sine (reference)
ref = np.sin(2 * np.pi * 440 * t).astype(np.float32)
save("reference_440hz.wav", ref)

# 2. Same sine with light noise (SNR ~40 dB)
noise = np.random.default_rng(42).normal(0, 0.01, N).astype(np.float32)
save("noisy_440hz.wav", ref + noise)

# 3. Silence
save("silence.wav", np.zeros(N, dtype=np.float32))

# 4. Clipped sine
clipped = np.clip(ref * 2.0, -1.0, 1.0).astype(np.float32)
save("clipped_sine.wav", clipped)

# 5. Different frequency (880 Hz)
save("sine_880hz.wav", np.sin(2 * np.pi * 880 * t).astype(np.float32))

# 6. Sine with harmonics (distorted)
distorted = (ref + 0.3 * np.sin(2 * np.pi * 880 * t) +
             0.15 * np.sin(2 * np.pi * 1320 * t)).astype(np.float32)
distorted /= np.max(np.abs(distorted))
save("distorted_440hz.wav", distorted)

# 7. Impulse (for RT60 testing)
impulse = np.zeros(N, dtype=np.float32)
impulse[0] = 1.0
# Simple exponential decay to simulate reverb
decay = np.exp(-5.0 * t).astype(np.float32)
reverb_ir = impulse * 0 + decay * 0.5 * np.sin(2 * np.pi * 440 * t).astype(np.float32)
reverb_ir[0] = 1.0
save("impulse_reverb.wav", reverb_ir)

print("Done.")
