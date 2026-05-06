# drumsynth

A simple LV2 drum synthesizer plugin with six designable engines:
- Kick
- Snare
- Hi-hat
- Tom
- Clap
- Sub/808

Now includes MIDI triggering support.

## Build

### Native build
```bash
make
```

### ARM64 / AArch64 build
```bash
make arm64
```

## Install locally
```bash
make install
```

This installs `drum-synth.lv2` to `~/.lv2/`.

## MIDI mapping
- C1 (36): Kick
- D1 (38): Snare
- F#1 (42): Hi-hat
- A1 (45): Tom
- D#1 (39): Clap
- C2 (48): Sub/808

## Notes
- You can still trigger each drum with its `gate_*` control input (0 -> 1 edge trigger).
- Use the tone/tune/frequency/drive controls for sound design.
