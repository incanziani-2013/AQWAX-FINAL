# AQWAX v1

Frutiger Aero inspired liquid audio processor built with JUCE 8.

## Requirements
- Windows
- JUCE 8
- Visual Studio 2022/2026 with Desktop development with C++
- CMake 3.22+

## Build with CMake

Open a terminal in this project folder:

cmake -S . -B build -DJUCE_DIR="C:/path/to/JUCE"
cmake --build build --config Release

The VST3 will be generated under the build tree and, with COPY_PLUGIN_AFTER_BUILD enabled, copied to the normal VST3 location.

## Parameters
WATER, AERO, GLOSS, SPACE, WIDTH, MIX, OUTPUT

This is AQWAX v1: a lightweight creative processor intended for beats and master-bus experimentation. It is not a transparent mastering limiter.

## Build en la nube (GitHub Actions)
Sin instalar nada: sube este proyecto a un repositorio de GitHub. En la pestana Actions,
el workflow "Build AQWAX VST3" compila en Windows y deja el VST3 como artifact "AQWAX-VST3".
