# drumsynth

6 plugins LV2 independientes: Kick, Snare, Hi-hat, Tom, Clap, Sub/808.

## MIDI
- **Sub/808**: se toca cromáticamente por nota MIDI y soporta **glide/slide** entre notas (`glide`).
- **Kick/Snare/Hi-hat/Tom/Clap**: MIDI solo dispara el golpe (no cromático).

## Parámetros
- Tone, Pitch
- Amp Decay / Amp Release
- Filter Decay / Filter Release
- Resonance
- Drive
- Glide (especialmente útil en Sub/808)
- Dist Type (Sub/808): 0=tanh, 1=hard clip, 2=fold-like, 3=valvula, 4=tubos, 5=tape saturation
- Dist Mix (Sub/808)
- Gate + MIDI In

## Build
```bash
make
```

## ARM64
```bash
make arm64
```
