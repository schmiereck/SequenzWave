# Prüfprotokoll – Meilensteine 1 und 2

## 2026-10-08: Lautsprecher-Prototyp

Der Benutzer hat die Kamera ausgebaut; GPIO17/18 bleiben ausschließlich für die noch nicht verdrahtete Analog-Sync-Schaltung reserviert. Der vorhandene Lautsprecher wird über ES8311, I²S GPIO12/13/15/16 und den NS4150B-Verstärker an TCA9554 P7 angesteuert. Es gibt drei gespeicherte UI-Modi: Aus, Metronom auf jedem Viertel (Step 1 akzentuiert), und lokale Sinustöne passend zu den Sequencer-Noten. Audio hat eine eigene statische Queue und Task; der MIDI-Timingpfad ruft nur einen nichtblockierenden Queue-Post auf. Im Modus Aus ist der Verstärker abgeschaltet und I²S hält an. Der ES8311-Treiber stammt aus dem offiziellen Waveshare-Beispiel; Quellenhinweis in `lib/ES8311/README.md`.

`g++ -std=c++11 -Wall -Wextra -Werror -Isrc tests/settings_codec.cpp src/storage/SettingsCodec.cpp -o build/settings-test.exe` und `build/settings-test.exe`: **PASS**, einschließlich V1/V2-Migration und neuem Tonmodus. Host-Tests für Touch, Sequencer, MIDI-Bytes und ClockSync sowie `.\.venv\Scripts\python.exe scripts/check_config.py`: **PASS**. `.\.venv\Scripts\python.exe -m platformio run -e waveshare_s3`: **SUCCESS**, RAM 116.000/327.680 Bytes, App-Flash 571.613/3.145.728 Bytes. `.\.venv\Scripts\python.exe -m platformio check -e waveshare_s3`: **PASSED**, 0 HIGH und 11 MEDIUM aus Fremdbibliotheken; zwei Warnungen zur Makroextraktion. `.\.venv\Scripts\python.exe -m platformio run -e waveshare_s3 -t upload --upload-port COM3`: **SUCCESS**, Flash-Hash bestätigt.

Serielle Lesung über etwa 11 Sekunden nach finalem Upload: zweimal `touch=ready`, `nvs=loaded`, `audio=ready`, `audio_drop=0`, `midi_drop=0`, stabiler `heap=261592`. Das bestätigt Initialisierung und Fortsetzung des UI-Loops, aber **keinen hörbaren Ton**, keine geprüfte Lautstärke und keinen am Gerät berührten Tonmodus. Der Benutzer muss Aus/Metronom/Noten, Reset-Persistenz, Wärme und Zusammenspiel mit MIDI/Touch am echten Board prüfen. Der DIN- und Sync-Aufbau sind weiterhin ausstehend.

Datum: 2026-10-08. **Meilenstein 1 und 2: Benutzer meldet stabilen Betrieb und bestätigt die Bedienung. Meilenstein 3: UART-Firmware und Hostprüfungen bestanden; elektrischer DIN- und Volca-Test ausstehend. Analog-Sync ist ein noch nicht aktivierter Entwurf.**

## Meilenstein 3: UART-MIDI, vorläufig

Der Benutzer bestätigt die Trennung der Vierergruppen als gut sichtbar und nicht störend. Bei 6 % Helligkeit bleibt die Anzeige laut Benutzer gut lesbar und das Gerät kühl; es liegt keine Temperatur- oder Strommessung vor.

Die V1- und V2-Schaltpläne wurden visuell auf J8 und die GPIO-Matrix geprüft: GPIO44 ist `ESP_RXD`, **V1 J8 Pin 27 / V2 J8 Pin 28**, ohne Belegung durch Display, Touch, Kamera, SD oder Audio. Der Benutzer hat die Platine als **Rev2.0** identifiziert; damit ist J8 Pin 28 maßgeblich. Er sah außerdem einen angeschlossenen Lautsprecher am Speaker-Port. Der Ausgang verwendet UART1 mit 31.250 Baud, 8N1 und eine feste Bytequeue. Die Schaltung gemäß MIDI CA-033 ist in [milestone3.md](milestone3.md) dokumentiert. Die MIDI-Hardware ist nicht angeschlossen; elektrische Übertragung und Volca-Reaktion sind noch ungetestet.

Für Analog-Sync wurden am Rev2.0-Schaltplan GPIO17/J8 Pin 15 und GPIO18/J8 Pin 17 identifiziert. Der Benutzer ist bereit, die ungenutzte Kamera abzustecken; das Abstecken selbst wurde noch nicht bestätigt. Das ClockSync-Modell wird zunächst nur als reiner Host-Code geprüft. GPIO-Interrupt, 5-V-Schaltung, Sync-Weitergabe und Transportkopplung sind **nicht** auf dem Board getestet oder aktiviert. Planung und Quellen: [clock_sync.md](clock_sync.md).

`g++ -std=c++11 -Wall -Wextra -Werror -Isrc tests/clock_sync.cpp src/clocksync/ClockSync.cpp -o build/clock-sync-test.exe` und `build/clock-sync-test.exe`: **PASS** für interne Stepdauer, 2/4 PPQN, Zwischenstep, Entprellung und Signalausfall. `.\.venv\Scripts\python.exe -m platformio run -e waveshare_s3`: **SUCCESS** (RAM 111.724/327.680 Bytes, App-Flash 540.661/3.145.728 Bytes; das noch nicht aufgerufene ClockSync-Modul ändert das Image nicht). `.\.venv\Scripts\python.exe -m platformio check -e waveshare_s3 --fail-on-defect high`: **PASSED**, 0 HIGH/11 MEDIUM aus Fremdbibliotheken und zwei Makroextraktions-Warnungen. Der Host-Touch-Test und `python scripts/check_config.py` bestanden. Diese neue Buildfassung wurde **nicht erneut geflasht**, da nur ein inaktives Modul und Dokumentation ergänzt wurden; die zuletzt geflashte UART-Firmware ist der Stand aus Commit `3d76884`.

Firmware-Build mit `.\.venv\Scripts\python.exe -m platformio run -e waveshare_s3`: **SUCCESS**, RAM 111.724/327.680 Bytes, App-Flash 540.661/3.145.728 Bytes. `.\.venv\Scripts\python.exe -m platformio check -e waveshare_s3 --fail-on-defect high`: **PASSED**, 0 HIGH, dieselben 11 MEDIUM-Warnungen in Fremdbibliotheken und zwei Warnungen zur Makroextraktion. `python scripts/check_config.py` und Host-Tests für Touch, Sequencer, Speicherformat einschließlich V1-Migration sowie MIDI-Bytekodierung (`g++ -std=c++11 -Wall -Wextra -Werror -Isrc ...`) bestanden. `platformio run -e waveshare_s3 -t upload --upload-port COM3`: **SUCCESS**, Flash-Hash verifiziert.

USB-Startprüfung nach Upload: `UI alive | touch=ready | PSRAM_heap=8386279 | heap=271132 | nvs=loaded | late_us=0 | skipped=0 | dropped=0 | midi_drop=0`. Ein kurzer Start/Stop-Lauf über die Debugschnittstelle erzeugte 29 Note-On- und 29 Note-Off-Logs; Status `late_us=903`, `skipped=0`, `midi_drop=0`. Das prüft die Aufrufe im Sequencer und die UART-Queue auf dem Board, **nicht** den elektrischen Pegel oder die tatsächlichen seriellen Bits am GPIO/DIN-Port. Die neue Settings-Bedienung und Persistenz des MIDI-Kanals wurden noch nicht durch Touch/Reset am Gerät bestätigt.

## Nachtrag: Vierergruppen, dunklere Anzeige und NVS

Der Benutzer bestätigt alle Funktionen der ersten M2-Firmware, einschließlich Lauflicht. Bei 10 % Helligkeit wird das Gerät nach seiner Beobachtung deutlich kühler und bleibt gut bedienbar. Die neue Version ergänzt je 10 Pixel Abstand zwischen den Step-Gruppen 1–4/5–8 und 9–12/13–16 sowie einen Helligkeitsbereich von 2–100 %. Ohne gespeicherten Wert startet die Anzeige bei 10 %.

Auf ausdrücklichen Benutzerwunsch werden Pattern-Steps (Note, Rest, Velocity, Gate), BPM und Helligkeit nun per NVS gespeichert. Zwei wechselnde 80-Byte-Slots mit Versionskennung und CRC32 erlauben Rückfall auf den letzten intakten Stand. Speicherung erfolgt nach zwei Sekunden ohne Änderung oder sofort über Save/Back. Play-Status und aktuell ausgewählter Bearbeitungs-Step werden nicht gespeichert.

Der Host-Test des Speicherformats prüft CRC-Fehler in jedem Byte, ungültige Werte, Slot-Fallback und Zählerüberlauf: bestanden. Firmware-Build: SUCCESS (RAM 111.180 Bytes, App-Flash 520.625 Bytes). PlatformIO-Cppcheck: PASSED, dieselben elf MEDIUM-Warnungen in Bibliotheken und keine HIGH-Befunde. Host-Tests für Engine, Touch und Konfiguration erneut bestanden. Upload auf COM3: SUCCESS mit Hashprüfung. Serieller Starttest: `UI alive | touch=ready | PSRAM_heap=8386295 | heap=273744 | nvs=defaults | late_us=0 | skipped=0 | dropped=0`. `nvs=defaults` ist beim ersten Start ohne gespeicherten Datensatz erwartet. Der Benutzer bestätigt anschließend, dass Pattern und weitere Einstellungen nach dem Reset gespeichert bleiben. Die Vierergruppierung und 2-%-Helligkeit wurden nicht ausdrücklich einzeln bestätigt.

## Meilenstein 2 und Helligkeit

Der Benutzer meldet zehn Minuten stabilen Betrieb am Netzteil und hat das Board wieder am PC angeschlossen. Erwärmung wurde subjektiv beobachtet; keine Temperaturmessung. Helligkeitsregelung ergänzt (GPIO6, 20-kHz-LEDC-PWM, 10–100 %, Standard 40 %). UI-Loop gibt jetzt etwa 5 ms an Idle ab. Ob das Gerät dadurch spürbar kühler wird, ist noch nicht bestätigt.

Implementiert: Pattern mit 16 Steps, 30–300 BPM, Start/Stop, Noten-/Oktavwahl, Rest, Gate, Velocity, Lauflicht, separate Sequencer-Aufgabe und abstrakte Mock-MIDI-Ausgabe. Keine DIN-Ausgabe und keine Speicherung.

Prüfungen:

- C++11-Engine-Test mit `-Wall -Wextra -Werror`: bestanden; einschließlich 10.000 Steps bei 137 BPM, Gate/Rest, Tempo, Stop, Edits aktiver Noten und längeren Scheduler-Aussetzern.
- Bestehender Touch-Host-Test und Konfigurationsprüfung: bestanden.
- Echter Firmware-Build: SUCCESS; RAM 110.956 Bytes, App-Flash ca. 510 kB.
- PlatformIO-Cppcheck: PASSED, unverändert elf MEDIUM-Bibliothekswarnungen, keine HIGH-Befunde. Einschränkung durch nicht extrahierte Toolchain-Defines bleibt bestehen.
- Upload auf bestätigtes Board COM3: SUCCESS, Hashprüfung bestanden.
- `scripts/serial_smoke.py --port COM3`: 65 ON/OFF-Paare bei Default-Pattern und 120 BPM; Note Off für letzte Note nach Stop bestätigt. Gemessene Step-Abstände jeweils 125.000 µs; normale Gates 94.000–94.002 µs (Soll 93.750 µs, Task-Raster 1 ms).
- Boardstatus im Test: `touch=ready`, `heap=274084`, `late_us=0`, `skipped=0`, `dropped=0`.

Diese kurze Messung verwendet Zeitstempel aus dem Mock-Task, nicht USB-Ankunftszeiten oder elektrische MIDI-Messung. Sie belegt keine allgemeine Jittergrenze. Kein Temperaturvergleich durchgeführt. UI-Abnahme für M2 sowie Standalone-Wiedergabe/Langzeitlauf bleiben offen.

## Historie Meilenstein 1 (damaliger Stand)

## Bestätigte Bedienprüfung

Der Benutzer bestätigt am 2026-10-08 für die Firmware aus Commit `1b619ba`:

- „MIDI Sequencer“, alle 16 Steps und Play werden im Querformat dargestellt.
- Antippen der äußeren Steps 1, 8, 9 und 16 markiert jeweils den richtigen Step.
- Die Schaltfläche wechselt beim Antippen korrekt zwischen Play und Stop.

Damit waren die grundlegende Displaydarstellung, die Touch-Transformation an den vier äußeren Steps und die Transport-Testschaltfläche am realen Gerät bestätigt. Später meldete der Benutzer zehn Minuten stabilen Betrieb am Netzteil und gab Meilenstein 2 frei. Die übrigen Steps und Rand-/Loslassverhalten wurden nicht einzeln protokolliert.

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
