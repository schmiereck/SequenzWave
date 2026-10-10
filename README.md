# SequenzWave

Standalone Touch-MIDI-Sequencer für das **Waveshare ESP32-S3-Touch-LCD-3.5-C**.

**Meilenstein 1:** Display und Touch am Board bestätigt. **Meilenstein 2:** 16-Step-Prototyp mit eigener Timing-Engine, Helligkeit und NVS-Speicherung am Board bestätigt. **Meilenstein 3:** UART-MIDI OUT implementiert; DIN-Schaltung und Volca-Test stehen noch aus. Den Prüfstand dokumentiert [docs/validation.md](docs/validation.md).

Die Platine im Gehäuse ist **Rev2.0**. Ein Lautsprecher ist angeschlossen; die OV5640-Kamera wurde ausgebaut. GPIO17/18 sind für Volca-Sync reserviert. Die [Sync-Hardware und Clock-Architektur](docs/clock_sync.md) sind dokumentiert, aber in der Firmware noch nicht aktiviert.

## Bedienung

- **Play/Stop:** Start immer bei Step 1; jeder Step ist eine Sechzehntelnote. Gold markiert die Wiedergabe, der türkise Rahmen die Bearbeitungsauswahl.
- **BPM −/+**: 30–300 BPM, Startwert 120.
- **Step antippen**, dann **Note −/+** (Halbton) oder **Oct −/+** (Oktave). MIDI-Noten 0–127; C4 = MIDI 60.
- **Rest/Enable:** Step pausieren oder wieder aktivieren. Die Note bleibt erhalten.
- **Settings:** Gate 5–100 %, Velocity 1–127, MIDI-Kanal 1–16, Helligkeit 2–100 %, **Ton** (Aus → Metronom → Noten) und **Volume** 0–100 %. Der Ton-Schalter oben wechselt bei jedem Antippen den Modus. Standard ist Aus. Metronom klickt auf Viertelnoten, mit höherem Ton auf Step 1. Noten gibt die MIDI-Noten lokal als einfachen synthetischen Ton wieder; Pausen bleiben still. Der Lautsprecher ist nur ein Monitor, kein Volca-Klangmodell. Volume 80 % entspricht dem bisherigen Codec-Pegel; 100 % erreicht 0 dB Codec-Verstärkung, ohne zusätzliche digitale Anhebung.
- Änderungen am Step werden beim nächsten Abspielen dieses Steps wirksam. Eine gerade klingende Note behält ihr ursprüngliches Note Off.
- Ein Tempo-Wechsel setzt das nächste Step-Intervall ab dem Änderungszeitpunkt neu an. Start/Stop ist unabhängig von der Step-Auswahl.
- Pattern, BPM, MIDI-Kanal, Helligkeit, Tonmodus und Lautstärke werden nach zwei Sekunden ohne weitere Änderung in NVS gespeichert. **Save** schreibt sofort; **Back** im Settings-Menü speichert ebenfalls sofort. Während „Saving...“ angezeigt wird, kann ein unmittelbarer Reset die letzte Änderung verlieren. Beim Neustart werden die letzten gültigen Werte geladen. Transport (Play/Stop) und ausgewählter Step werden nicht gespeichert.

Die USB-Diagnose spiegelt die UART-MIDI-Ereignisse als `MOCK t=… ON/OFF ch=… note=… vel=…`. Der Zeitstempel kommt aus dem Sequencer-Task; die spätere USB-Ankunft ist keine Timingmessung. Der UART-Ausgang liegt auf GPIO44 (`ESP_RXD`), **J8 Pin 27 bei V1 oder Pin 28 bei V2**, und benötigt die Schaltung aus [Meilenstein 3](docs/milestone3.md). Optional lässt sich für Prüfungen `p` (Start) oder `s` (Stop) über den Monitor senden. Reguläre Bedienung erfolgt vollständig über Touch.

## Helligkeit und Wärme

Die Hintergrundbeleuchtung wird über GPIO6 mit 20-kHz-PWM geregelt. Der Regler zeigt das PWM-Tastverhältnis, keine kalibrierte Leuchtdichte. Der Benutzer meldet, dass das Display bei 6 % noch gut lesbar ist und das Gerät kühl bleibt; eine Temperaturmessung liegt nicht vor. Die UI schläft zwischen Durchläufen etwa 5 ms. Die Engine bleibt in einem separaten Task aktiv. CPU-Takt und Stromversorgung wurden nicht verändert.

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
2. Board über USB-C-Datenkabel verbinden. Für reine Firmware- und UI-Tests ist kein DIN-MIDI-Aufbau erforderlich.
3. Port mit `python -m platformio device list` ermitteln; unten `COMx` ersetzen.
4. Build und Upload starten:

```powershell
python -m platformio run -e waveshare_s3 -t upload --upload-port COMx
python -m platformio device monitor --port COMx --baud 115200
```

Nach einem erfolgreichen Build liegt das Image unter `.pio/build/waveshare_s3/firmware.bin`; PlatformIO schreibt auch Bootloader und Partitionstabelle mit den korrekten Offsets. Das App-Image nicht allein an Adresse 0 schreiben.

Falls Upload nicht verbindet: BOOT gedrückt halten, RESET kurz drücken, BOOT loslassen; den möglicherweise geänderten COM-Port erneut bestimmen. Bei Übertragungsfehlern `upload_speed = 115200` versuchen. Monitor vor erneutem Upload schließen.

Die App wartet nicht auf USB. Alle fünf Sekunden erscheint `UI alive` mit Touchstatus, PSRAM, freiem Heap und `midi_drop` oder ein konkreter `INIT ERROR`. Diese periodische Ausgabe ist bewusst auch nach spätem Öffnen des Monitors sichtbar. Ein erfolgreicher SPI-Start bestätigt noch keine physische LCD-Kommunikation.

## Hardware-Abnahme

- Kaltstart und Reset: lesbares 480×320-Bild, richtige Farben, kein Flackern.
- Alle 16 Steps mehrfach berühren, besonders 1/8/9/16; Markierung und Auswahltext müssen übereinstimmen.
- Play/Stop mehrfach betätigen: Beschriftung wechselt genau einmal pro Tap.
- Finger loslassen und an Rändern bewegen: keine hängenbleibenden/versetzten Touch-Ereignisse.
- Physischer PSRAM = 8388608 Bytes (Startprüfung); `PSRAM_heap` ist wegen Verwaltungsdaten etwas kleiner. Touch ready und stabiler Heap über mindestens 10 Minuten prüfen.
- Betrieb mit USB-Netzteil ohne PC; keine Startblockade.
- Ergebnis, Boardrevision, Orientierung und verwendeten Commit in docs/validation.md eintragen.

## Architektur

- `src/sequencer`: PatternModel und SequencerEngine, reines C++ ohne Arduino/LVGL. 64-Bit-Zeit in Mikrosekunden, absolute Step-Grenzen ohne aufsummierten Rundungsfehler.
- `src/midi`: abstrakte MidiOutput-Schnittstelle für Note On/Off; UART1-Ausgang mit fester Bytequeue und USB-Diagnosespiegel.
- `src/clocksync`: hardwareunabhängiges Zeitmodell für interne BPM und analoge 2/4-PPQN-Impulse. Noch nicht mit GPIOs oder der Wiedergabe verbunden.
- `src/app`: separater FreeRTOS-Task auf Core 0, Priorität 3, statische Befehls-/Ereignisqueues und Zustandskopie. Keine Heap-Allokation oder USB-Ausgabe im Timingpfad.
- `src/hardware`: Display, Touch, LVGL-Port und Backlight-PWM.
- `src/audio`: ES8311/I²S-Lautsprecher mit eigener PCM-Task und begrenzter Eventqueue. Der MIDI-Timing-Task stellt nur nichtblockierende Ereignisse ein.
- `src/storage`: versioniertes NVS-Format mit zwei Prüfsummen-Slots; schreibt nur aus dem UI-Loop.
- `src/ui`: LVGL-Ansicht auf dem Arduino-Loop-Task. Befehle gehen in eine Queue; Lauflicht liest einen geschützten Snapshot.

Details und Timinggrenzen: [Meilenstein 2](docs/milestone2.md).

## Nächste Schritte

1. Beim bestätigten Rev2.0-Board die Pin-1-Orientierung von J8 prüfen und die MIDI-OUT-Schaltung aus [Meilenstein 3](docs/milestone3.md) aufbauen.
2. DIN-Ausgang elektrisch prüfen und mit dem Volca FM Note On/Off, Kanal und Gate testen; Timing messen.
3. Die bereits ausgebaute OV5640 getrennt lassen, SYNC-IN-/OUT-Schaltungen aus [Meilenstein 3](docs/milestone3.md) aufbauen und elektrisch prüfen. Danach GPIO-ISR, Pulsweitergabe, Transport und UI mit dem [ClockSync-Modul](docs/clock_sync.md) verbinden.
4. M4 erweitert die einfache NVS-Speicherung um mehrere Patterns, Kopieren/Löschen und Fehlerbehandlung.

Weitere Details: [Hardware und Quellen](docs/hardware.md), [Entscheidungen](docs/decisions.md), [Prüfprotokoll](docs/validation.md), [Projektkonventionen](AGENTS.md).

Eine Open-Source-Lizenz für eigenen Projektcode ist vor Veröffentlichung noch festzulegen. Abhängigkeiten behalten ihre jeweiligen Lizenzen; sie sind nicht ins Repository kopiert.
