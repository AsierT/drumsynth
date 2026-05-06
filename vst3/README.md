# DrumSynth VST3 (Windows x86)

Este branch incluye una base VST3 para Windows x86 (compatible para cargar en Windows 11 x64 en DAWs que soporten plugins VST3 de 32 bits, normalmente via bridge).

## Requisitos
- Visual Studio con toolset C++
- Steinberg VST3 SDK
- CMake 3.21+

## Build x86 (Win32)
```powershell
cmake -S . -B build-win32 -G "Visual Studio 17 2022" -A Win32 -DVST3_SDK_ROOT=C:/ruta/vst3sdk
cmake --build build-win32 --config Release
```

Salida esperada: `DrumSynth.vst3` en la carpeta de build.

## Nota
Esta implementación es un esqueleto inicial de VST3 para rama separada. Si quieres, en el siguiente paso te porto todo el DSP de las 6 voces LV2 al procesador VST3.
