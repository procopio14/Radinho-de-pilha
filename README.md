# Radinho (VST3)
Um knob. Zero para limpo, 100% para radinho antigo.

## Compilar
Requisitos: CMake 3.22+, Git e
- Windows: Visual Studio 2022 (Desktop C++) ou Build Tools
- macOS: Xcode Command Line Tools

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target Radinho_VST3

Resultado: build/Radinho_artefacts/Release/VST3/Radinho.vst3

## Instalar
- Windows: C:\Program Files\Common Files\VST3\
- macOS: ~/Library/Audio/Plug-Ins/VST3/   (se bloquear: xattr -cr Radinho.vst3)

Ableton Live 12: Preferences > Plug-ins > ative "Use VST3 System Folders" e Rescan.

## Ajustar o som
Source/PluginProcessor.cpp: updateFilters() (frequencias, saturacao, volume) e radioNoise() (chiado/estalos).
