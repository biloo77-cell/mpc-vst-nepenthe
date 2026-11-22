# Upstream and local changes

This is a modified version of **Nepenthe** by Amalgamated Signals (https://amalgamatedsignals.com/nepenthe), a
velvet-noise reverb, licensed GPL-3.0 (`LICENSE`, the upstream file unchanged). The port is licensed under the same
terms. It is not affiliated with or endorsed by Amalgamated Signals.

- Upstream: `nepenthe-source.zip` from the site above, version 1.0.1 (`Nepenthe.jucer`), files dated 2023-12-13.
- `upstream/Source/`: **unmodified** copies of the DSP: `Delay`, `Echoes`, `Equalizer`, `Mixer`, `Reverberator`,
  `Timer` (`.h` and `.cpp`). All but `Reverberator.cpp` and `Echoes.cpp` are compiled as they are.
- `upstream/reference/`: the JUCE files, unmodified and not compiled, as the reference for the port files below:
  `Parameters`, `Processor`, `Editor`, `LookAndFeel`, `Fonts`, `Nepenthe.jucer`.

## Port files (2026-11-22)

- `src/shim/Parameters.h`, `src/Parameters.cpp`: upstream's `Parameters` without JUCE. Same fields, constants and
  formulas (`initialize`, `update`, `getFilterK`, `getAmplitude`); the six values come from the engine instead of
  JUCE's parameter tree.
- `src/engine.cpp`: upstream's `Processor` as an `mpc_engine()` for the Open MPC wrapper: the same objects and the same
  block processing (mono sum, EQ, pre-delay, the two reverbs, width, dry/wet), at 44.1 kHz in 128-frame blocks, with
  flush-to-zero during the block (upstream's `ScopedNoDenormals`) and upstream's 100 ms restart rule (`Timer`).
  - Parameters: upstream's six, ranges, defaults and steps. TIME travels 0..1000 and is turned into seconds by JUCE's
    `NormalisableRange(0.5, 8, 0.01, skew 0.7)` formula, so the slider position matches upstream's; the others are whole
    numbers as upstream's `AudioParameterInt`. Value text as upstream's text boxes (`2.20`, `30`, no unit).
  - Kept as upstream: in the width stage the right channel is computed from the already-widened left
    (`right = self * right + cross * left` after `left` changed), so WIDTH 0 is not mono.
- `src/opt/Echoes.cpp` (replaces upstream's in the build): upstream's file with one change, the echo pattern's random
  generator gets a fixed seed instead of `std::random_device`, so every instance and every project load has the same
  pattern (same distribution; reproducible bounces; the Open MPC host test requires two instances to match). User
  decision, 2026-11-22.
- `src/opt/Reverberator.cpp` (replaces upstream's in the build; its header is upstream's): the same algorithm with the
  loops written for the device. The echo loop (1000 echoes x 128 samples x 2 channels per block, nearly all of the
  plugin's cost) runs in NEON on armv7 and aarch64, 16 samples per step; a write that crosses the end of the circular
  buffer is done as two straight runs instead of a test per sample; the read / clear / feedback loop runs four samples
  at a time between wraps. Same operations in the same order, multiplies and adds not fused (`-ffp-contract=off` in
  `vst/vst.json`): `tests/ref_check.cpp` checks every output sample bit-identical to upstream's `Reverberator`.
- `vst/skin/make_skin.py` (Pillow): draws `vst/art/` and writes `vst/layout.conf`. Upstream's editor at MPC size: six
  vertical sliders drawn after upstream's `LookAndFeel::drawLinearSlider` (track, gradients, thumb, colours) as
  filmstrips, names in Karla (upstream's font, `vst/skin/fonts/`, OFL), live values under them. Added: the title
  "Nepenthe Reverb" at the bottom in Playball (the font of upstream's web page, OFL), coloured with the sliders'
  three colours. MPC limit: a slider strip is at most 12288 px, so each 416 px slider has 29 positions (the value
  text is exact). Edit the script, not the layout.

## Latency

None beyond upstream's own pre-delay: the dry path is not delayed.
