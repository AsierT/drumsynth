# drumsynth

6 plugins LV2 independientes: Kick, Snare, Hi-hat, Tom, Clap, Sub/808.

## MIDI
- **Sub/808**: se toca cromáticamente por nota MIDI y soporta **glide/slide** entre notas (`glide`).
- **Kick/Snare/Hi-hat/Tom/Clap**: MIDI solo dispara el golpe (no cromático).

## Parámetros
- Pitch (-36 a +36 semitonos enteros)
- Tone
- Amp Decay / Amp Release
- Amp Attack
- Filter Type, Cutoff, Resonance
- Filter Env Amount
- Filter Attack / Filter Decay / Filter Release
- Drive
- Glide (especialmente útil en Sub/808)
- Dist Type: 0=clean, 1=soft, 2=clip, 3=drive, 4=asym, 5=fold
- Dist Amount
- Gate + MIDI In

## Build
```bash
make
```

## ARM64
```bash
make arm64
```

## S2400 / insert-compatible ARM64
Builds six separate LV2 bundles under `s2400-lv2/`. Each bundle has audio inputs
at ports 0/1, audio outputs at ports 2/3, MOD-style metadata, and a binary that
exports only one LV2 descriptor.

```bash
make arm64-s2400
```

Check runtime dependencies:

```bash
readelf -d s2400-lv2/drumsynth-kick.lv2/drumsynth_kick.so | grep NEEDED
strings -a s2400-lv2/drumsynth-kick.lv2/drumsynth_kick.so | grep -E 'GLIBC_|GLIBCXX_|GCC_' | sort -V | uniq
```

## Vibraphone
There is also a duophonic vibraphone LV2. MIDI Note On triggers the played note
plus one automatic harmony note calculated from Root, Scale, and Interval. It
keeps only two notes active at a time and includes:

- Harmony Level, Harmony Direction, Scale Snap
- Tone, Mallet Hardness, Strike Noise, Bar Decay, Velocity Sens
- Amp Attack / Decay / Sustain / Release
- Tremolo Rate / Depth, Width, Gain
- Spring Mix / Decay / Tone / Drive / Shake
- Delay Mix / Time / Feedback, Tape Tone, Wow Flutter, Tape Age, Head Mode

```bash
make arm64-vibraphone-s2400
```


## VST3 (Windows x86)
- Guía de compilación en Windows 11: `vst3/README.md`.
