> **WIP:** Proyecto en desarrollo. La API, el sonido, la metadata LV2 y los targets de build pueden cambiar.

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
make s2400
```

## ARM64
```bash
make arm64-s2400
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

## License
GPL-2.0-only.
