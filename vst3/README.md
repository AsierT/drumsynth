# DrumSynth VST3 (Windows x86) — guía de compilación en Windows 11 x64

Esta rama contiene un **scaffold VST3** para generar un plugin **x86 (Win32)** que pueda usarse en Windows 11 x64 en DAWs que soporten plugins de 32 bits (normalmente con bridge interno o externo).

> Estado actual: esta versión VST3 es una base inicial (estructura Processor/Controller + parámetros), no un port completo del DSP LV2.

## 1) Requisitos

1. **Windows 11 x64**.
2. **Visual Studio 2022** (Community/Pro/Enterprise) con workload:
   - **Desktop development with C++**
   - MSVC v143 toolset
   - Windows 10/11 SDK
3. **CMake 3.21+**.
4. **Git**.
5. **Steinberg VST3 SDK** descargado localmente.

## 2) Preparar entorno

### 2.1 Clonar el repo y cambiar a la rama VST3
```powershell
git clone <URL_DE_TU_REPO> drumsynth
cd drumsynth
git checkout vst3-windows-x86
```

### 2.2 Obtener VST3 SDK
Descarga el SDK oficial de Steinberg y descomprímelo, por ejemplo en:

```text
C:\SDKs\vst3sdk
```

Asegúrate de que exista una ruta tipo:

```text
C:\SDKs\vst3sdk\CMakeLists.txt
```

## 3) Compilar Win32 (x86)

> Importante: ejecuta estos comandos en **x64 Native Tools Command Prompt for VS 2022** o en PowerShell con herramientas de VS disponibles.

Desde la carpeta `drumsynth/vst3`:

```powershell
cmake -S . -B build-win32 -G "Visual Studio 17 2022" -A Win32 -DVST3_SDK_ROOT=C:/SDKs/vst3sdk
cmake --build build-win32 --config Release
```

Si todo va bien, el `.vst3` se generará dentro de `build-win32` (ruta exacta según configuración de CMake/VS).

## 4) Instalar en Windows

Copia el bundle `.vst3` compilado a una de estas rutas:

- Usuario actual:
  ```text
  %LOCALAPPDATA%\Programs\Common\VST3
  ```
- Todos los usuarios:
  ```text
  C:\Program Files\Common Files\VST3
  ```

Luego reescanea plugins en tu DAW.

## 5) Verificación rápida en DAW

1. Abrir DAW en Windows 11.
2. Forzar **rescan** de VST3.
3. Insertar `DrumSynth` en una pista.
4. Verificar que:
   - el plugin carga,
   - expone parámetros,
   - recibe MIDI.

## 6) Solución de problemas

### Error: `Set -DVST3_SDK_ROOT to the Steinberg VST3 SDK root`
La ruta de `-DVST3_SDK_ROOT` es incorrecta o no apunta al root del SDK.

### Error de generador Visual Studio / plataforma
Verifica que usas:
- `-G "Visual Studio 17 2022"`
- `-A Win32`

### El DAW x64 no carga VST3 x86
No todos los DAWs soportan bridging de plugins de 32 bits. Revisa si tu DAW requiere bridge externo.

## 7) Comandos útiles

Limpiar build:
```powershell
Remove-Item -Recurse -Force .\build-win32
```

Reconfigurar desde cero:
```powershell
cmake -S . -B build-win32 -G "Visual Studio 17 2022" -A Win32 -DVST3_SDK_ROOT=C:/SDKs/vst3sdk
```

---
Si quieres, el siguiente paso es portar completamente el motor DSP de las 6 voces LV2 al Processor VST3 en esta rama.
