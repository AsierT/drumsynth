# drumsynth

A simple LV2 drum synthesizer plugin with six designable engines:
- Kick
- Snare
- Hi-hat
- Tom
- Clap
- Sub/808

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

## Notes
- Trigger each drum with its `gate_*` control input (0 -> 1 edge trigger).
- Use the tone/tune/frequency/drive controls for sound design.
