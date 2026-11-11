# ShredMaxx2000

A distorting character compressor in a road-worn amp head. It can level gently like a vintage opto box, grab transients like a fast FET unit, or flatten a drum room into a wall.

## How the gain staging works

There is no threshold knob. The compressor has a fixed internal threshold, and **Input** decides how hard you hit it. Turning Input up pushes the signal further into both gain reduction and the harmonic circuits. **Output** is the makeup gain, and runs through a Class-A style output amp that stays clean at normal levels and rounds off gently when pushed.

With Input at 0 dB, a signal around -18 dBFS sits right at the threshold.

## Front panel

| Control | What it does |
|---|---|
| **Input** | -12 to +36 dB of drive into the compressor and the harmonic circuits. |
| **Attack** | 50 microseconds (fast enough to clamp a single waveform cycle) up to 30 ms. |
| **Release** | 50 ms up to 3.5 s. In 10:1 the opto cell stretches this out (see below). |
| **Ratio** | Eight settings that each change the knee, the threshold and the detector, not just the slope. |
| **Output** | -24 to +24 dB of makeup gain through the Class-A output amp. |
| **Headroom** | 4 to 28 dB. Sets where the internal circuits run out of room, above the fixed reference. Low headroom = dirtier and more saturated. High headroom = cleaner. |
| **Mix** | Dry/wet blend for built-in parallel compression. The dry signal is delay-matched, so blending never sounds phasey. |

### Ratio settings

| Setting | Character |
|---|---|
| **1:1** | No compression at all. The unit is a pure colour box: Input, Headroom and the Dist circuits still shape the tone. |
| **2:1, 3:1** | Ultra-soft parabolic knee, 30 dB and 26 dB wide, with a lower threshold and an averaging detector. Transparent levelling. |
| **4:1, 6:1** | Medium-soft knee and a peak-leaning detector. Punch and transient control. |
| **10:1** | Switches to a separate opto-cell detector. Its release is two-stage and program-dependent: the first half recovers at the Release setting, then a slow tail that stretches the longer and harder the cell has been working. On sustained material with Release at maximum the tail runs out to about 20 seconds. The OPTO CELL lamp glows brighter as the cell "charges". |
| **20:1** | Hard-knee peak limiter. |
| **KABOOM** | Over-limits (louder input actually gets slightly quieter) and adds an instantaneous brick wall after the VCA. Built for smashing drum room mics. |

### Eardrum section (how the compressor listens)

| Switch | What it does |
|---|---|
| **HPF** | 100 Hz high-pass in the detector only, so kick and sub-bass stop pumping the whole mix. |
| **Band Beater** | +12 dB boost at 6 kHz in the detector only, so harsh upper mids and sibilance get pulled down first. Works as a de-esser on vocals. |
| **Stereo Link** | Sums the two channels' control voltages so both sides get the same gain change and the stereo image stays put. Off = dual mono. |

### Audio section (what you hear)

| Switch | What it does |
|---|---|
| **HPF** | 80 Hz, 18 dB/octave high-pass on the audio itself. |
| **Dist Deux** | Tube-like, mostly 2nd-order (even) harmonics. About 3% THD at the reference level, rising as you push Input. |
| **Dist Trois** | Tape-like, mostly 3rd-order (odd) harmonics that round off transients. About 1.7% THD at the reference level, rising as you push Input. |

### Meters and lamps

- **Gain Reduction VU**: 0 to 20 dB of gain reduction.
- **THD 1% / THD 3%**: light up when the harmonic content of the signal passes 1% and 3%. These are measured live from the audio, not guessed from the settings.
- **Opto Cell**: lit in 10:1, brighter as the opto memory builds up.

## Level safety

The output can never go above 0 dBFS. Everything below -0.9 dBFS passes untouched; anything hotter is rounded off smoothly instead of clipping hard. If anything ever went wrong inside (a bad value from the host, for example) the plugin mutes and resets that block instead of making noise.

## Building and installing

1. Create a new GitHub repository called `ShredMaxx2000`.
2. Upload `CMakeLists.txt`, `README.md` and the `Source` folder.
3. Add a file at `.github/workflows/build.yml` with the contents of `build.yml` from this folder.
4. Wait for the green check on the Actions tab, then download **ShredMaxx2000-VST3** from Artifacts.
5. Extract it and copy the `ShredMaxx2000.vst3` folder into `C:\Program Files\Common Files\VST3`, then rescan plugins in your DAW.

Built with [JUCE](https://juce.com/) 9.0.5.
