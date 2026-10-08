# SequenzWave

Standalone Touch-MIDI-Sequencer für das **Waveshare ESP32-S3-Touch-LCD-3.5**, zunächst Meilenstein 1.

**Stand: Firmware gebaut, geflasht und grundlegende Bedienung am Gerät bestätigt.** Der Benutzer bestätigt Querformat-Darstellung, Auswahl der Steps 1/8/9/16 und Play/Stop. Build, Host-Test, Konfigurationsprüfung und Speicherinitialisierung sind erfolgreich. PlatformIO-Cppcheck meldet elf Warnungen in Fremdbibliotheken, keine Befunde hoher Schwere. Langzeittest und vollständige Hardware-Abnahme bleiben offen; siehe [Prüfprotokoll](docs/validation.md).

## Enthalten

- LVGL-Oberfläche im Querformat 480 × 320: „MIDI Sequencer“, 2 × 8 Steps, Auswahlmarkierung, 120 BPM, Play/Stop.
- Play/Stop ändert nur die Beschriftung. Keine Sequencer-Engine, Notenausgabe oder Speicherung.
- ST7796 über Arduino_GFX, FT6336 über SensorLib, Reset über TCA9554.
- USB-Diagnose, PSRAM-/Flash-Prüfung, sichtbare Meldung bei fehlendem Touch.

## Build unter VS Code / PlatformIO

PlatformIO IDE installieren, diesen Ordner öffnen und Environment `waveshare_s3` bauen. Beim ersten Build ist Internetzugang nötig. Alternativ in einem Terminal mit PlatformIO Core 6.1.18:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install platformio==6.1.18
.\.venv\Scripts\python.exe -m platformio run -e waveshare_s3
.\.venv\Scripts\python.exe -m platformio check -e waveshare_s3
```

PlatformIO in der isolierten `.venv` installieren, damit seine Paketversionen andere Python-Anwendungen nicht verändern. Eine Aktivierung ist bei diesen expliziten Aufrufen nicht nötig. Auch bei den folgenden Flash-/Monitor-Befehlen `python` durch `.\.venv\Scripts\python.exe` ersetzen. Der PlatformIO-Paketcache liegt projektlokal in `.pio-core` (nicht in Git), damit die Agentensitzung dort schreiben kann.

Die Konfiguration pinnt Espressif32 6.10.0, Arduino-ESP32 2.0.17, LVGL 8.4.0, Arduino_GFX 1.5.5, SensorLib 0.3.1 und TCA9554 0.1.2. Sie ist ein zu validierender Port der Arduino-Referenz, keine vom Hersteller gelieferte PlatformIO-Konfiguration. `esp32-s3-devkitc-1` dient nur als generisches SoC-Profil; Flash, OPI-PSRAM und Pins sind überschrieben.

## Flashen und Diagnose

1. Boardetikett prüfen: **3.5 / 3.5-C**, nicht **3.5B**. Revision V1/V2 im [Hardwareprotokoll](docs/validation.md) eintragen.
2. Board über USB-C-Datenkabel verbinden. Kein DIN-MIDI-Aufbau für diesen Test erforderlich.
3. Port mit `python -m platformio device list` ermitteln; unten `COMx` ersetzen.
4. Build und Upload starten:

```powershell
python -m platformio run -e waveshare_s3 -t upload --upload-port COMx
python -m platformio device monitor --port COMx --baud 115200
```

Nach einem erfolgreichen Build liegt das Image unter `.pio/build/waveshare_s3/firmware.bin`; PlatformIO schreibt auch Bootloader und Partitionstabelle mit den korrekten Offsets. Das App-Image nicht allein an Adresse 0 schreiben.

Falls Upload nicht verbindet: BOOT gedrückt halten, RESET kurz drücken, BOOT loslassen; den möglicherweise geänderten COM-Port erneut bestimmen. Bei Übertragungsfehlern `upload_speed = 115200` versuchen. Monitor vor erneutem Upload schließen.

Die App wartet nicht auf USB. Alle fünf Sekunden erscheint `UI alive` mit Touchstatus, PSRAM und freiem Heap oder ein konkreter `INIT ERROR`. Diese periodische Ausgabe ist bewusst auch nach spätem Öffnen des Monitors sichtbar. Ein erfolgreicher SPI-Start bestätigt noch keine physische LCD-Kommunikation.

## Hardware-Abnahme

- Kaltstart und Reset: lesbares 480×320-Bild, richtige Farben, kein Flackern.
- Alle 16 Steps mehrfach berühren, besonders 1/8/9/16; Markierung und Auswahltext müssen übereinstimmen.
- Play/Stop mehrfach betätigen: Beschriftung wechselt genau einmal pro Tap.
- Finger loslassen und an Rändern bewegen: keine hängenbleibenden/versetzten Touch-Ereignisse.
- Physischer PSRAM = 8388608 Bytes (Startprüfung); `PSRAM_heap` ist wegen Verwaltungsdaten etwas kleiner. Touch ready und stabiler Heap über mindestens 10 Minuten prüfen.
- Betrieb mit USB-Netzteil ohne PC; keine Startblockade.
- Ergebnis, Boardrevision, Orientierung und verwendeten Commit in docs/validation.md eintragen.

## Architektur

`src/main.cpp` verbindet `hardware` und `ui`. `hardware` besitzt I²C, LCD, Touch, den LVGL-Port und die Zeitfortschreibung. `ui` erstellt ausschließlich LVGL-Widgets. Der RGB565-Zeilenpuffer belegt 19.200 Bytes internen RAM; SPI-Flush ist synchron. PSRAM wird für spätere Erweiterungen initialisiert und geprüft.

Später kommen `SequencerEngine`, `PatternModel`, `MidiOutput` und `Storage` als getrennte Module hinzu. Timing läuft dann unabhängig von LVGL in einer eigenen Aufgabe. Die UI übermittelt Befehle und zeigt Zustandskopien an. Synthesizer-Sound bleibt außerhalb des Pattern-Modells.

## Nächste Schritte

1. Erledigt: Abhängigkeiten installiert und Firmware erfolgreich gebaut.
2. Boardrevision bestätigen, flashen und die Hardware-Abnahme durchführen.
3. Erst danach Meilenstein 2: testbares Pattern-Modell, Transport/Timing und Mock-MIDI.
4. Vor Meilenstein 3: UART-Anschluss am konkreten Board bestätigen und DIN-Ausgang elektrisch auslegen und messen.

Weitere Details: [Hardware und Quellen](docs/hardware.md), [Entscheidungen](docs/decisions.md), [Prüfprotokoll](docs/validation.md), [Projektkonventionen](AGENTS.md).

Eine Open-Source-Lizenz für eigenen Projektcode ist vor Veröffentlichung noch festzulegen. Abhängigkeiten behalten ihre jeweiligen Lizenzen; sie sind nicht ins Repository kopiert.
