# Klangbild Echo

Bandecho mit drei Wiedergabeköpfen im Klangbild-Vintage-Look – AU, VST3 und Standalone für macOS (Logic Pro).

Ein Aufnahmekopf schreibt auf eine Bandschleife, drei Wiedergabeköpfe lesen im Abstand 1×, 2× und 3× der
eingestellten Zeit. Die Sättigung sitzt vor dem Band, jede Wiederholung wird dunkler, und Zeitänderungen
gleiten wie bei einem trägen Motor (die Tonhöhe zieht mit). Im Fenster läuft das Band, die aktiven Köpfe leuchten mit dem Echo.

## Regler

| Regler | Aufgabe |
|---|---|
| **Zeit** | Abstand von Kopf 1: 40 – 700 ms |
| **Rückkopplung** | Anzahl der Wiederholungen; ganz aufgedreht kippt das Echo in Selbstoszillation (bleibt begrenzt) |
| **Köpfe** | 1 · 2 · 3 · 1+2 · 2+3 · 1+3 · alle – für rhythmische Muster |
| **Notenwert** | Bei „Im Takt“: 1/4, 1/8 punktiert, 1/8, 1/8 Triole, 1/16 nach Logic-Tempo |
| **Bandalter** | Wow & Flutter und Höhenverlust eines alten Bandes |
| **Sättigung** | Wie heiß das Band angefahren wird |
| **Ton** | Helligkeit der Wiederholungen |
| **Mischung** | Echo-Anteil |
| **Stereo-Bild** | Mono oder Stereo (Köpfe links/rechts verteilt, leicht versetzter Gleichlauf) |
| **Ausgang** | Mit Direktsignal oder nur Echo (für einen Send/Aux) |

## Presets

Standard · Slapback · Soul – Achtel im Takt · Blues – warmes Echo · Dreikopf-Rhythmus · Punktierte Achtel ·
Altes Band – leiernd · Dub – kurz vorm Kippen · Send – 100 % Echo

## Installation

Über den [Klangbild Installer](https://github.com/Guitoli1/KlangbildInstaller):

```
curl -fsSL https://raw.githubusercontent.com/Guitoli1/KlangbildInstaller/main/install.sh | bash
```

## Build

JUCE 8 / CMake. GitHub Actions baut auf macOS 15 ein Universal Binary (arm64 + x86_64), signiert ad-hoc und prüft mit `auval`.
