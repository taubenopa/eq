# Crazy Visual EQ

Visueller 8-Band-Parametrik-EQ als **VST3 (nur Windows, 64 Bit)** fuer FL Studio.
Gebaut mit JUCE, automatisch per GitHub Actions.

## Features

- 8 Baender: Bell, Low Shelf, High Shelf, High-Pass 12/24, Low-Pass 12/24, Notch
- +-24 dB Gain, 20 Hz - 20 kHz, Q 0.1 - 18
- Live-Spektrum (8192er FFT, log-Achse, Tilt-Kompensation), Pre-/Post-EQ, Peak-Falloff
- Gesamtkurve + Einzelkurve des gewaehlten Bands, Bandbreiten-Anzeige
- Klaviatur-Leiste unter der Frequenzachse, Hover zeigt Frequenz, Note, Cent und dB
- Stereo-Pegelmeter mit Peak-Hold und Clip-Anzeige, Output-Gain
- Animationen: Aurora-Hintergrund, Sterne, audioreaktive Partikel, Scanner-Laser,
  pulsierende Nodes, Shockwaves beim Ein-/Ausschalten, Regenbogen-Spektrum
- CRAZY-Schalter reduziert alle Animationen auf ein ruhiges Minimum
- Fenster ist in der Groesse veraenderbar (falls der Host es erlaubt)

## Bedienung

| Aktion | Wirkung |
|---|---|
| Node ziehen | Frequenz + Gain (bei HP/LP/Notch: horizontal = Freq, vertikal = Q) |
| Mausrad auf Node/Graph | Q (Bandbreite) |
| Doppelklick auf Node | Band an/aus |
| Doppelklick ins Leere | naechstes freies Band dorthin setzen |
| Rechtsklick auf Node | Filtertyp, an/aus, Reset |
| Knoepfe unten | exakte Werte (Doppelklick = Reset, Eingabe per Klick auf den Wert) |
| RANGE | Anzeigebereich des Graphen 6 / 12 / 18 / 24 dB |

## Das .vst3 bekommen (ohne lokale Installation)

1. Neues GitHub-Repo anlegen und **alle Dateien dieses Ordners** hochladen
   (inklusive `.github/workflows/build.yml` - dieser eine Pfad ist von GitHub vorgeschrieben,
   alles andere liegt flach im Hauptordner).
2. Tab **Actions** -> Workflow **Build VST3 (Windows)** laeuft automatisch (oder "Run workflow").
3. Nach ca. 5-10 Minuten: Lauf oeffnen -> unter **Artifacts** `CrazyVisualEQ-VST3-Windows` herunterladen.
4. Fuer ein Release: Tag `v1.0.0` pushen, dann haengt die ZIP automatisch am Release.

## In FL Studio installieren

1. Den Ordner `Crazy Visual EQ.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren.
2. FL Studio: **Options -> Manage plugins -> Find installed plugins** (Verify plugins aktivieren).
3. Plugin im Mixer-Slot ueber **Crazy Visual EQ** laden.

## Lokal bauen (optional)

Voraussetzung: Visual Studio 2022 (Desktop C++), CMake >= 3.22, Git.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Lizenz-Hinweis

JUCE steht unter AGPLv3 bzw. kommerzieller Lizenz. Wenn du das Plugin weitergibst,
muss das Repo entsprechend (A)GPL-kompatibel oeffentlich sein, oder du brauchst eine JUCE-Lizenz.
