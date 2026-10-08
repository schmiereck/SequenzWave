# Prüfprotokoll – Meilenstein 1

Datum: 2026-10-08. **Firmware-Build bestätigt; Hardware-Abnahme offen.**

## Bestätigte Bedienprüfung

Der Benutzer bestätigt am 2026-10-08 für die Firmware aus Commit `1b619ba`:

- „MIDI Sequencer“, alle 16 Steps und Play werden im Querformat dargestellt.
- Antippen der äußeren Steps 1, 8, 9 und 16 markiert jeweils den richtigen Step.
- Die Schaltfläche wechselt beim Antippen korrekt zwischen Play und Stop.

Damit sind die grundlegende Displaydarstellung, die Touch-Transformation an den vier äußeren Steps und die Transport-Testschaltfläche am realen Gerät bestätigt. Noch nicht ausdrücklich geprüft: alle übrigen Steps und Rand-/Loslassverhalten, zehn Minuten Dauerbetrieb sowie Kaltstart am USB-Netzteil ohne PC. Meilenstein 1 bleibt bis zur vollständigen Abnahmecheckliste offen; Meilenstein 2 wurde nicht begonnen.

## Erster Hardwarestart auf COM3

Der Benutzer bestätigt das Boardetikett ESP32-S3-Touch-LCD-3.5-C mit FT6336, ST7796, TCA9554 und 8 MB PSRAM sowie die Zuordnung zu COM3. PCB-Revision weiterhin offen. Esptool identifiziert ESP32-S3 Chiprevision v0.2; dies ist nicht die PCB-Revision.

Upload mit `.\.venv\Scripts\python.exe -m platformio run -e waveshare_s3 -t upload --upload-port COM3` erfolgreich, geschriebene Daten per Hash verifiziert. Der erste Start zeigte einen Fehler in unserer PSRAM-Größenprüfung: Arduino `ESP.getPsramSize()` liefert Heap-Kapazität nach Verwaltungsabzug. Die Prüfung verwendet jetzt nach `psramFound()` die physische Größe aus `esp_spiram_get_size()` (Header des gepinnten ESP-IDF im Arduino-Core).

Erneuter Build und Upload erfolgreich (17,90 Sekunden; RAM 105.684 Bytes, App-Flash 520.089 Bytes). Statische Analyse erneut PASSED mit denselben elf MEDIUM-Bibliothekswarnungen und derselben Einschränkung der Compiler-Makroextraktion. Host- und Konfigurationsprüfung erneut bestanden.

Serieller Lesetest über 16 Sekunden nach Neustart: wiederholt `UI alive | touch=ready | PSRAM_heap=8386295 | heap=280148`. Damit sind physische 8-MB-PSRAM-/16-MB-Flash-Prüfung, Expanderinitialisierung, Touch-Erkennung und LVGL-Initialisierung durchlaufen. Der Heap bleibt in diesem kurzen Beobachtungsfenster gleich. Das ersetzt keinen Zehn-Minuten-Test. Displaydarstellung, reale Touch-Koordinaten und Standalone-Kaltstart müssen noch vom Benutzer geprüft werden.

Die folgenden Abschnitte bleiben als Verlauf erhalten; Angaben zu noch nicht erfolgtem Flashen oder unbekanntem Modell sind durch diesen Abschnitt überholt.

## Aktueller Prüflauf nach Installation und Berechtigungsänderung

- Benutzer-Erstbuild: SUCCESS in 309,89 Sekunden; Agent-Wiederholungsbuild: SUCCESS in 8,23 Sekunden.
- PlatformIO 6.1.18 in `.venv`, Arduino 2.0.17, Xtensa GCC 8.4.0. Alle vier Bibliotheken wurden in den konfigurierten Versionen aufgelöst.
- Statischer RAM: 105.684 / 327.680 Bytes (32,3 %). App-Flash: 520.097 / 3.145.728 Bytes (16,5 %). Firmwareimage unter `.pio/build/waveshare_s3/firmware.bin` erzeugt.
- `platformio check -e waveshare_s3 --fail-on-defect high`: PASSED, 0 HIGH / 11 MEDIUM / 0 LOW. Zehn Warnungen über nicht im Konstruktor initialisierte Member in SensorLib, eine über einen gleichnamigen geerbten Member in Arduino_GFX. Keine Befunde im eigenen `src`. Keine Bibliotheken verändert oder Befunde unterdrückt.
- Einschränkung der Analyse: zweimal `Failed to extract toolchain defines!`; automatisch ermittelte Compiler-Makros möglicherweise unvollständig. PASSED bedeutet bei der gewählten HIGH-Schwelle nicht warnungsfrei.
- Host-Test erneut mit `g++ -std=c++11 -Wall -Wextra -Werror -Isrc tests/touch_transform.cpp -o build/touch-test.exe` kompiliert und ausgeführt: bestanden. `python scripts/check_config.py`: bestanden.
- `python -m pip check`: No broken requirements found. Der globale Abhängigkeitskonflikt wird nicht mehr gemeldet.
- Paketdownload in Agentensitzung: PlatformIO tool-cppcheck erfolgreich heruntergeladen und installiert. Die frühere Netzwerksperre blockiert diesen Vorgang nicht mehr.
- Git-Staging und `git diff --cached --check` funktionieren jetzt; keine Whitespace-Fehler. Projektdateien werden als initialer Stand versioniert, Toolchains und Buildartefakte bleiben ignoriert.
- COM3 identifiziert: USB VID:PID 303A:1001 (Espressif). Modell und Boardrevision sind damit nicht bestimmt. Port sechs Sekunden ohne gesetztes DTR/RTS gelesen; keine Daten empfangen. Kein Gerät geflasht.
- Display-, Touch-, PSRAM- und Standalone-Hardwaretests weiterhin nicht durchgeführt. CI weiterhin nicht ausgeführt.

Die Flash-Prozentzahl bezieht sich auf den 3-MB-App-Slot. Die generische PlatformIO-Boardbezeichnung N8/No PSRAM beschreibt die Vorlage; das Projekt überschreibt Flashgröße und Speichermodus. Reale Speichererkennung bleibt ein Hardwaretest.

Aktuelle PlatformIO-Befehle verwenden `.\.venv\Scripts\python.exe -m platformio`. Die folgenden Abschnitte dokumentieren den ursprünglichen Stand und sind durch diesen Nachtrag hinsichtlich Build, Downloads und Python-Konflikt überholt.

## Ursprünglicher Prüflauf

Nachtrag: Der Benutzer hat PlatformIO 6.1.18 erfolgreich global installiert; die Version ist lokal erkennbar. Dabei entstand ein Konflikt zwischen `sse-starlette 3.4.2` und dem auf 0.46.2 zurückgesetzten `starlette`. README verwendet deshalb jetzt eine isolierte `.venv`. `core_dir = .pio-core` vermeidet den zusätzlich beobachteten Schreibfehler beim Anlegen von `C:/Users/thomas/.platformio`. ESP32-Paketdownload und Firmware-Build sind weiterhin offen; die folgende Tabelle beschreibt den ursprünglichen Prüflauf.

| Prüfung | Ergebnis |
|---|---|
| Offizielle Dokumentation und LVGL-Beispiel gelesen | durchgeführt |
| V1-/V2-Schaltplan | Text untersucht; vollständige visuelle Netzkontrolle offen |
| PlatformIO-Firmware-Build | blockiert: `No module named platformio` |
| Installation PlatformIO 6.1.18 | fehlgeschlagen: kein erreichbarer Paketdownload |
| ESP32-Compiler lokal | nicht vorhanden; Arduino15 enthält nur Arduino-/Builtin-Pakete |
| Cppcheck 2.14, eigener Quellcode | bestanden, keine gemeldeten Defekte |
| C++11 Host-Test, `-Wall -Wextra -Werror` | kompiliert und bestanden |
| Koordinatentransformation | Ecken, ungültige Werte, Bijektion aller 153.600 Pixel bestanden |
| PlatformIO-INI und Flash-Partitionen | Offline-Plausibilitätsprüfung bestanden |
| Firmwaregröße / Linker / Bibliotheksauflösung | nicht geprüft |
| Flashen / LCD / Touch / Standalone / Langzeitbetrieb | **nicht durchgeführt** |
| CI | Workflow angelegt, nicht ausgeführt |
| Git | Repository initialisiert; Staging/Commit durch Schreibschutz auf `.git/index.lock` blockiert |

## Reproduktion

```powershell
New-Item -ItemType Directory -Force build
g++ -std=c++11 -Wall -Wextra -Werror -Isrc tests/touch_transform.cpp -o build/touch-test.exe
.\build\touch-test.exe
python scripts/check_config.py
cppcheck --enable=warning,performance,portability --error-exitcode=1 --suppress=missingIncludeSystem --inline-suppr -Iinclude -Isrc src
python -m platformio run -e waveshare_s3
```

Die installierte Strawberry-Cppcheck-Ausgabe suchte `std.cfg` an einem nicht existierenden Buildpfad. Eine lokale Kopie von `cppcheck.exe` mit `cfg/std.cfg` unter `.tools/cppcheck` konnte ausgeführt werden; der dokumentierte erfolgreiche Lauf benutzte diese Kopie. Die Analyse lief ohne heruntergeladene Arduino-/LVGL-Header und ersetzt deshalb weder API-Typprüfung noch Linken.

`git clone` zum Hersteller schlug über den vorgegebenen Netzwerkproxy fehl. Auch `pip install --target .tools/python platformio==6.1.18` konnte kein Paket laden. Diese Beschränkung wurde nicht umgangen. Referenzquellen konnten mit dem Webwerkzeug gelesen werden.

COM3 wurde von Windows gelistet; die Geräteidentifikation über CIM war nicht erlaubt. Daher ist nicht festgestellt, ob ein ESP32 angeschlossen ist. Kein Port wurde geöffnet und kein Gerät geflasht.

`git add` wurde mit `Permission denied` auf `.git/index.lock` abgewiesen. Die Projektdateien liegen vollständig im Arbeitsverzeichnis, sind aber noch unversioniert. Nach Aufhebung des Metadaten-Schreibschutzes: `git add .`, `git diff --cached --check`, anschließend einen initialen Commit erstellen. Es wurde kein Commit vorgetäuscht.

## Noch einzutragende Hardware-Abnahme

- Etikett/SKU/Revision: offen
- Firmware-Commit und erfolgreicher Build: offen
- LCD-Ausrichtung/Farben/Backlight: offen
- Touch-Ecken, alle Steps, Loslassen und Play/Stop: offen
- Flash-/PSRAM-Erkennung und Heap nach 10 Minuten: offen
- Kaltstart und Betrieb am USB-Netzteil: offen

Abnahmekriterium: erfolgreicher echter Firmware-Build plus vollständige Checkliste aus README. Erst dann Meilenstein 1 als abgeschlossen markieren.
