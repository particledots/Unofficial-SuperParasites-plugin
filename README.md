# Unofficial SuperParasites — AU / VST3 port (work in progress)

A plugin wrapper around the **SuperParasites** firmware DSP: Clouds, Parasites (Oliverb, Resonestor) and Beat Repeat / Spectral Clouds
merged into one eight-mode module. This is the firmware that third-party modules such as Typhoon and Supercell run, so it can be compared
against those. Not a Mutable Instruments product, and not made by or endorsed by any of the module makers.
Don't contact Emilie Gillet or Mutable Instruments about it.

## Limiter switch (version 0.3.0)
A new **LIMITER** button (parameter `limiter`) sits at the end of the button row. **On (the default) is exactly the 0.2.0 sound**: the soft limiter after Output Level keeps loud sounds from going past 0 dBFS.
**Off** removes it: the output is then the engine plus the Output Level gain and nothing else, so loud wet sounds can go past 0 dBFS (by tens of dB at high Output Level settings). Only two safety things
remain: NaN becomes silence and values are held inside +/-16 so a runaway can never reach the host as infinity. Use Output Level, or a limiter after this plug-in, to keep it in range.
Switching glides over about 10 ms, so there is no click. Presets and projects saved before 0.3.0 have no switch and load with the limiter **on**, i.e. they sound as they did in 0.2.0.
Why it exists: the limiter is an instant curve (no attack, no release, no look-ahead), so on very loud sounds it acts like gentle distortion, which can be heard; the switch lets you turn that off and manage the level yourself.

## Output Level (version 0.2.0)
**What was wrong.** Loading any plug-in of this family made the sound quieter than before it was loaded. With Dry/Wet at 0 (nothing but the input) I measured the output at
**-9.04 dB** relative to the input in the six modes that have a dry path (Granular, Pitch shifter / time stretch, Looping delay, Spectral madness, Oliverb, Spectral clouds), at every
host rate tested (44.1, 48, 96 kHz) and all four qualities. The cause is in the firmware code, not the wrapper: its dry/wet crossfade table starts at 0.707 (-3 dB) and its
output conversion halves the signal (x 0.5, -6 dB) before the 16-bit output. The module's analogue output stage presumably makes that up; a plug-in has none.

**The fix.** A new **OUTPUT LEVEL** knob (parameter `output_level`, -24 to +24 dB, default **+9.03 dB** = 20 x log10 (2 x sqrt 2)) sits after the engine, followed by a **soft limiter**.
With the default, a fully dry signal comes out at the level it went in. The limiter passes everything below -0.9 dBFS unchanged and bends louder peaks towards 0 dBFS, which they
never exceed; it is an instant curve (no delay, no look-ahead), a safety net against clipping rather than a mastering limiter. The gain glides (about 10 ms) so moving or automating the knob does not click.
The engine and firmware code are untouched. The stage lives in `src/OutputStage.h` and is called from `processBlock`.

**What it does not do.**
* **Beat repeat and Resonestor have no dry path in the firmware**, so "fully dry equals the input" does not apply to them. Beat repeat forces its blend to wet and the input always goes through
  the slice player (the firmware applies a 1.2 x, +1.6 dB, gain to it that the other modes' dry signal does not get); Resonestor's Dry/Wet knob is a distortion amount. Measured at the default Output Level, input -18 dBFS:
  Beat repeat **+1.57 dB** (16-bit), Resonestor **-1.61 dB** (16-bit); in the low-fi qualities Beat repeat is -0.2 dB (stereo 8-bit) and Resonestor -2.6 dB. I left those as they are: whether they should be
  trimmed to match is your call.
* **Beat repeat in Mono 8-bit (low-fi) gives no output at all** in this test (-330 dB, at every Dry/Wet setting). It is in the firmware code path and was there before this change; I have not looked into it.
* Loud dry signals are bent slightly by the firmware's own soft limit, before this stage: Looping delay, dry, input -6 dBFS: -0.06 dB; -1.9 dBFS: -0.16 dB; -0.4 dBFS: -0.22 dB.
* The firmware's output is 16-bit and is 9 dB lower than the input before this gain is applied, so the gain also raises the firmware's own 16-bit rounding noise by 9 dB (it does not add any). I have not measured or listened for it in quiet passages.
* **Presets and projects saved before 0.2.0 have no Output Level**, so they load at the +9.03 dB default and come out about 9 dB louder than they did. That is the fix, but if you
  had turned the channel up to compensate, turn it back down.

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
| Output Level (version 0.2.0) | not on the module: makes up the firmware's output attenuation (-24 to +24 dB, default +9.03 dB), followed by a soft limiter; see the Output Level section |
| Limiter (version 0.3.0) | not on the module: soft limiter after Output Level, on by default; off lets loud sounds go past 0 dBFS |

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

## Verified and not verified

**Limiter switch (version 0.3.0): built on GitHub Actions (macOS runner), every step passed, run 39075612299, commit `abbb25d`** (all earlier tests, the extended level tests and both mutation checks, the universal arm64 + x86_64 AU and VST3 build, signing, packaging). I can see each step's pass / fail, not its log text. Not yet loaded in Logic and not listened to.

**Limiter switch (version 0.3.0) - measured and tested here (Linux, g++, October 2026), not listened to**
* `tests/level_test.cpp`: 10485 checks, 0 failed (the 0.2.0 checks plus the switch): with the limiter on, a loud signal never passes 1.0; off, the same signal comes out at exactly the Output Level gain (within 0.0001) and
  its peak is past 1.5; a quiet signal is identical on and off; NaN and infinity never get through when off; switching glides (no step above 2 % of the whole move) and settles exactly, and switching back on gives exactly the 0.2.0 value.
* `tests/proc_level_test.cpp`: 31 checks, 0 failed, on the real processor class: the parameter exists and is on by default; a preset saved without it loads with the limiter on; with Output Level at +24 dB and a loud wet input
  (four modes) the peak is 1.0000 with the limiter on and well past 1.0 with it off, so the switch really reaches the sound.
* Mutation checks: `tests/mutation_check.sh` now breaks the stage in 7 ways (the earlier 4 plus switch ignored one way and the other, and the off-guard removed) and `tests/wiring_mutation_check.sh` in 6 ways (the earlier 3 plus
  switch not read one way and the other, and default wrong); every broken copy was caught.
* Not tested: how the switch sounds or looks in Logic, whether the button fits the panel as intended (the row is 606 of 612 pixels wide), Logic's handling of the new parameter in an existing project (the plug-in code is unchanged).

**Output Level (version 0.2.0): every step passed on the macOS runner, run 39056541692, commit `9cf851c`** (all earlier tests, the four new level steps, the universal arm64 + x86_64 AU and VST3 build, signing, packaging). I can see each step's pass / fail, not its log text. Artifact `SuperParasites-macOS`.

**Output Level (version 0.2.0) - measured and tested here (Linux, g++, October 2026), not listened to**
* `tests/level_test.cpp`: 10,477 checks, 0 failed. Fully dry (Dry/Wet = 0) at the default Output Level, engine + stage output equals the input level within 0.1 dB in each of the six dry-path modes, at 44.1 / 48 / 96 kHz
  host, every quality the plug-in has, input -18 dBFS (and -6 dBFS at 48 kHz); worst deviation 0.064 dB. The limiter: identity up to its threshold, odd-symmetric, monotonic, slope 1 at the threshold,
  never above 1.0 for any input including infinity, NaN becomes 0. The gain is exact to 0.001 dB at -24, -6, 0, +9.03 and +12 dB; a step from +9.03 to -24 dB glides with no jump. No clipping: with a 0.95-amplitude input, all modes
  and qualities, Dry/Wet 0 / 0.5 / 1, Output Level at the default and at +24 dB, no sample exceeds 1.0; without the limiter the engine's wet peaks at the default gain would reach about 2.3 x full scale.
* `tests/proc_level_test.cpp`: 24 checks, 0 failed, on the real plug-in processor class (the same `PluginProcessor.cpp` as the AU / VST3, compiled against JUCE 8.0.6, run headless): the parameter exists and its default shows as "+9.03 dB";
  the six dry-path modes come out within 0.013 dB of the input at the defaults; setting Output Level to -24 / -9.03 / 0 / +6 dB moves the output by exactly that much (within 0.02 dB); loud input at +24 dB in every mode stays at or below 1.0;
  a preset saved without an Output Level loads at +9.03 dB (and keeps its other settings), also into an instance whose knob had been turned to 0 dB.
* **Mutation checks (the tests fail when the level is broken).** `tests/mutation_check.sh` breaks `OutputStage.h` in four ways (gain removed, default 8.0 dB instead of 9.03, wrong dB-to-gain factor, limiter removed) and the level test failed for every one
  (17, 13, 19 and 4,035 failed checks), while passing on the unbroken code. `tests/wiring_mutation_check.sh` breaks `processBlock` in three ways (stage not called, level parameter not read, parameter default 0 dB) and the whole-plug-in test failed each
  time (9, 9 and 8 failed checks); the source is restored afterwards. Both are workflow steps.
* The existing suite still passes with the new CMake targets (`engine_test`); the engine and firmware code are not touched.
* The editor source (the fifth knob in the second row) compiles against JUCE 8.0.6 with the rest of the plug-in. **How it looks is not checked: I have not seen the editor.**
* **Not verified:** how it sounds; that the level you read on Logic's meters / faders now matches; the 16-bit noise floor at +9 dB in quiet passages; the knob row in Logic; behaviour when automated; any
  match with the real module's output level; Beat repeat and Resonestor levels against a real module.

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
