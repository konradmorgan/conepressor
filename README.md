# ConePressor – LA-2A + Fairchild 670 in einem Plugin

Umschaltbarer Kompressor (VST3 / AU / Standalone), gebaut mit JUCE 8.

## Module
**LA-2A (Opto):** Peak Reduction, Compress/Limit-Schalter, Opto-typisches programmabhängiges Release (ca. 60 ms bis 2,5 s)
**Fairchild 670 (Variable-Mu):** Threshold, Time Constant 1-6 (inkl. programmabhängiger Stellungen 5 und 6), Ratio steigt mit dem Pegel
**Gemeinsam:** Input, Output (Makeup), Mix (Parallelkompression), Tube (Röhrensättigung), GR-Meter
**Gag:** Katzenbild im GUI, schrumpft mit der Gain Reduction

## Bauen

### Voraussetzungen
- **Windows:** Visual Studio 2022 Community (Workload "Desktopentwicklung mit C++"), CMake, Git
- **macOS:** `xcode-select --install`, `brew install cmake`
- **Linux:** `sudo apt install build-essential cmake git libasound2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libgl1-mesa-dev`

### Kompilieren (im Ordner ConePressor)
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Beim ersten Mal lädt CMake JUCE automatisch von GitHub (dauert etwas).

## In die DAW einbinden
Nach dem Build kopiert JUCE das Plugin automatisch (COPY_PLUGIN_AFTER_BUILD):

| System | VST3-Ziel | AU-Ziel |
|---|---|---|
| Windows | `C:\Program Files\Common Files\VST3` (Terminal / VS als Administrator starten, sonst manuell kopieren) | – |
| macOS | `~/Library/Audio/Plug-Ins/VST3` | `~/Library/Audio/Plug-Ins/Components` |
| Linux | `~/.vst3` | – |

Manuell findest du die Dateien unter `build/ConePressor_artefacts/Release/VST3/ConePressor.vst3` (bzw. `.../AU`, `.../Standalone`).

Dann in der DAW einen Plugin-Rescan machen (Ableton: Einstellungen > Plug-ins > Rescan; Logic: AU-Manager; Cubase/Reaper/FL: Plugin-Pfad scannen).

**macOS Gatekeeper:** Für den Eigengebrauch reicht ad-hoc-Signierung:
```
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/VST3/ConePressor.vst3
codesign --force --deep -s - ~/Library/Audio/Plug-Ins/Components/ConePressor.component
```
Falls die DAW es trotzdem blockt: `xattr -cr <pfad-zum-plugin>`

## Testen
1. Zuerst die **Standalone**-Version starten (kein DAW-Scan nötig), Audiodatei/Eingang durchschicken.
2. Kostenloses **pluginval** (Tracktion) laufen lassen (Strictness 5+), damit keine Crashes in der DAW passieren.
3. Gain-Staging mit Output-Regler ausgleichen und per Mix-Regler A/B hören.

## Nächste Schritte
- 2x Oversampling für die Sättigung (juce::dsp::Oversampling) gegen Aliasing
- Sidechain-HPF, Stereo-Link-Regler, Mid/Side-Modus für den Fairchild
- Eigene Presets, größere/gestaltete Knobs (LookAndFeel)
- Eigene Messung/Tuning der Kennlinien gegen Hardware oder bekannte Emulationen

## Lizenz-Hinweis
JUCE ist für Eigennutzung bzw. unterhalb einer Umsatzgrenze kostenlos, sonst AGPL oder Bezahllizenz. Vor Verkauf/Vertrieb prüfen. Für VST3 gelten die Steinberg-Bedingungen (in JUCE 8 ist das SDK MIT-lizenziert eingebunden).

## Ohne Installation: Online bauen mit GitHub Actions
1. Kostenloses Konto auf github.com anlegen, neues Repository erstellen (privat ist okay).
2. Den kompletten Inhalt dieses Ordners hochladen (*Add file > Upload files*). Wichtig: auch der versteckte Ordner `.github` muss mit hoch. Am einfachsten den Ordner per Drag and Drop in den Browser ziehen.
3. Oben auf den Tab **Actions** gehen. Der Build startet automatisch (ca. 10-15 Min.).
4. Fertigen Lauf anklicken und unten unter **Artifacts** `ConePressor-Windows` bzw. `ConePressor-macOS` herunterladen.
5. Zip entpacken und die `.vst3` in den Plugin-Ordner kopieren (siehe oben), DAW-Rescan.
