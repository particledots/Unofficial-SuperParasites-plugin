# Unofficial SuperParasites — AU / VST3 port (work in progress)

A plugin wrapper around the **SuperParasites** firmware DSP: Clouds, Parasites (Oliverb, Resonestor) and Beat Repeat / Spectral Clouds
merged into one eight-mode module. This is the firmware that third-party modules such as Typhoon and Supercell run, so it can be compared
against those. Not a Mutable Instruments product, and not made by or endorsed by any of the module makers.
Don't contact Emilie Gillet or Mutable Instruments about it.

## Easiest: let GitHub build it
`.github/workflows/build.yml` builds the AU and VST3 on a macOS runner (universal arm64 + x86_64, macOS 11+).
Download `SuperParasites-macOS` from the latest run's Artifacts (Actions tab). `INSTALL.txt` inside covers the install.

## Build it yourself (macOS)
```
brew install cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```
It shows up in Logic as "particledots: Unofficial SuperParasites". The AU is declared as a *MIDI-controlled effect* (`aumf`),
so MIDI note-ons can press CAPTURE.

Headless DSP test (any OS): `cmake --build build --target engine_test && build/engine_test`

## The eight modes (in the order the module's mode button steps through them)

1. Granular  2. Pitch shifter / time stretch  3. Looping delay  4. Spectral madness  (the four Clouds modes)
5. Oliverb  6. Resonestor  (from Parasites)  7. Beat repeat  8. Spectral clouds  (from Beat Repeat)

The editor shows a one-paragraph note for the selected mode. Those notes were read from the firmware source, not checked against a real module.

## Parameters (mapped from the panel)

| Plugin | Module |
|---|---|
| Position, Size, Density, Texture | the four main knobs |
| Pitch | the pitch knob, through the module's own detented curve (about ±24 semitones; the text shows the semitones) |
| Dry/Wet, Stereo Spread, Feedback, Reverb | the module's single **BLEND** knob has four roles; here each role has its own knob |
| Freeze | the freeze button / gate |
| Capture (button) and MIDI note-on | the CAPTURE button / input: seeds a grain, taps tempo, strikes the Resonestor, starts a Beat Repeat slice, re-rolls Spectral clouds |
| Gate (button) and held MIDI note | the gate input; in Spectral madness it glitches the spectrum |
| Reverse | granular reverse |
| Pitch Follows MIDI | optional: the last MIDI note transposes Pitch (middle C = no change). Not applied to Beat repeat's own pitch parameter |
| Mode | the eight modes above |
| Quality | stereo 16-bit, mono 16-bit, stereo 8-bit (low-fi), mono 8-bit (low-fi), in the module's order |

Switching Mode (to or from Spectral, Spectral clouds, Oliverb or Resonestor) or Quality clears the buffer and unfreezes, as on the module.

## How it works / honest limits

* The module DSP is called as the firmware does: 32 kHz (16 kHz in the low-fi qualities), 32-frame blocks, 16-bit samples, the module's
  118 784-byte + 65 408-byte memory blocks. A windowed-sinc resampler converts to and from the host rate. The dry signal also goes
  through this path, as on the module, so nothing above about 16 kHz survives.
* Each knob is smoothed per block with the firmware's own coefficients (from `supercell/cv_scaler.cc`).
* The firmware runs a background `Prepare()` step in its main loop. The wrapper runs it 16 times per 32-frame block.
* Left out: CV inputs, calibration, VU meter, input/output mute and bypass, sample-memory save/load, the pot/UI code, the hardware drivers.
* The Beat repeat pitch parameter is the pitch knob position with nothing patched to V/OCT. The module's real V/OCT calibration isn't modelled.
* **Verified here (Linux, October 2026):** the DSP and wrapper compile with g++ and run at 44.1 / 48 / 96 kHz in all eight modes and all four
  qualities, with finite, bounded, non-silent output; Freeze keeps sounding after the input stops; switching mode and quality while running does
  not crash; extreme knob settings stay bounded (`engine_test`). The plugin sources compile against the JUCE 8.0.6 headers without warnings.
  Spectral clouds is silent by design when Position is at 0 (every band is muted).
* **Not verified:** the macOS/AU build (until the GitHub run passes), loading in Logic on either Mac, MIDI behaviour in a real host,
  and **how closely it sounds like a real Typhoon or Supercell**. Nobody has done an A/B against a module yet.
* The UI is a basic custom panel. It is functional rather than polished.

## Where the source came from

* `third_party/supercell/` is `supercell/dsp/*` and `supercell/resources.*` from `patrickdowling/superparasites`, commit `2e9ed7c`.
  One change: the hardware-only `#include "supercell/drivers/debug_pin.h"` line was removed from `granular_processor.cc`.
* `third_party/stmlib/` is the subset of `mqtthiqs/stmlib` at commit `8ab2aaee77cbacb47b646d46d22ee5d358effe2d` (the version SuperParasites pins) that the DSP needs.
* The engine allocates the processor from zeroed memory, because some of it is never initialised by the code itself:
  on the module it lives in zeroed global memory.

## Licensing

The SuperParasites README states the STM32F code is MIT-licensed, and the source files carry MIT headers: Emilie Gillet (Clouds, stmlib),
Matthias Puech (Parasites additions), Julius Kammerl (`kammerl_player`, Beat Repeat), Patrick Dowling (Supercell port). The repository has no separate LICENSE file.
Copyright notices are kept in the file headers; keep them if you redistribute (see also `third_party/stmlib/LICENSE`).
The hardware designs are not used here.
The wrapper code in `src/` has no licence assigned yet. JUCE is used under its own licence terms (AGPLv3 / commercial); check them before distributing builds.
