# Planung: analoger Volca-Sync

## Befund und GPIOs

Der Benutzer hat **ESP32-S3-Touch-LCD-3.5-C Rev2.0** bestätigt. Am Speaker-Anschluss ist ein Lautsprecher angeschlossen; die Audioleitungen bleiben reserviert. Laut [Rev2.0-Schaltplan](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5_Rev2.0.pdf) sind am J8-Header verfügbar:

| Funktion | ESP32-S3 | J8 Rev2.0 | Bisherige Verbindung |
|---|---:|---:|---|
| MIDI OUT | GPIO44 | 28 | `ESP_RXD`; bereits UART1 TX |
| SYNC IN (vorgesehen) | GPIO17 | 15 | OV5640 `CAM_VSYNC` |
| SYNC OUT (vorgesehen) | GPIO18 | 17 | OV5640 `CAM_HREF` |
| 5 V für Sync-Ausgang | VBUS | 1 | nur bei USB-Versorgung verfügbar |
| 3,3 V | VCC3V3 | 31/32 | Logikversorgung |
| GND | GND | 29/30 | gemeinsamer Bezug für Analog-Sync |

**Voraussetzung:** Das ungenutzte OV5640-Flachbandkabel ist abgesteckt, bevor GPIO17/18 als Sync-Pins aktiviert oder verdrahtet werden. Ein nicht initialisierter Kameratreiber garantiert keine elektrisch hochohmigen Kameraausgänge. Die Firmware aktiviert diese beiden Pins derzeit **nicht**. Die Kamera darf bei Sync-Betrieb nicht wieder angeschlossen werden.

Andere Headerpins sind belegt: GPIO9–11 durch microSD, GPIO12–16 durch den Audio-Codec (Lautsprecher später nutzbar), GPIO43 durch den Boot-UART, GPIO0 als Boot-Strap, GPIO4 durch den Expander-Interrupt. Die übrigen freien Header-I/Os gehören ebenfalls zur Kamera. GPIO17/18 sind nach Abstecken der Kamera die kleinste saubere Änderung ohne Verlust von SD, Audio oder USB. Auf dem Rev2.0-Header liegen ihre Positionen weit genug von MIDI GPIO44 getrennt; Pin-1-Orientierung am realen Board vor Anschluss prüfen.

## Signal und Schutzschaltung

Das [offizielle Volca-FM-Handbuch](https://cdn.korg.com/us/support/download/files/f160e38a8112b463f6546dad35091bd5.pdf) nennt für SYNC OUT 5-V-Impulse von 15 ms; SYNC IN ignoriert dann die interne Step-Clock. Die Sync-Polarität ist am Volca umstellbar. Die globale Option `SyncStp 1` sendet einen Impuls je Step (**4 PPQN** bei Sechzehnteln), die Werkseinstellung `SyncStp 2` einen Impuls je zwei Steps (**2 PPQN**). Vor dem Test alle Volcas auf gleiche Auflösung und steigende Polarität stellen oder die Auswahl im Sequencer passend setzen.

**SYNC IN:** 3,5-mm-Mono-Buchse, Spitze = Signal, Schaft = GND. Von der Spitze über 22 kΩ zu einem Knoten, von dort 33 kΩ und 1 nF nach GND. Bei 5 V liegen am Knoten ungefähr 3,0 V. Eine 3,6-V-Zenerdiode (Kathode am Knoten) begrenzt höhere Fehlpegel. Der Knoten geht auf den Eingang eines mit 3,3 V versorgten [SN74LVC1G17-Schmitt-Puffers](https://www.ti.com/lit/ds/symlink/sn74lvc1g17.pdf); dessen Ausgang geht an GPIO17. Der Puffereingang ist bis 5,5 V tolerant und trennt den ESP-Pin zusätzlich vom externen Signal. 100 nF zwischen Puffer-VCC und GND. Die konkrete Zener- und Puffer-Bauteilvariante samt Pinbelegung vor Aufbau im Datenblatt prüfen; Pegel am GPIO messen, bevor verbunden wird.

**SYNC OUT:** GPIO18 an Eingang eines mit USB-VBUS (5 V) versorgten [SN74AHCT1G125](https://www.ti.com/product/SN74AHCT1G125). Seine TTL-kompatible Eingangslogik erkennt 3,3 V sicher als HIGH. 10 kΩ vom Eingang nach GND halten den Ausgang beim Reset LOW; `/OE` gemäß Datenblatt aktivieren. Puffer-Ausgang über 1 kΩ zur Spitze einer zweiten 3,5-mm-Mono-Buchse, Schaft an GND; 100 nF direkt an Puffer-VCC/GND. Der Serienwiderstand begrenzt den Kurzschlussstrom auf etwa 5 mA. **Nur bei USB-Versorgung ist hier 5 V vorhanden.** Für späteren Batteriebetrieb wäre eine separate 5-V-Versorgung/Ausgangsstufe erforderlich. Ausgangspegel unter Last messen, bevor ein Volca angeschlossen wird. Nicht direkt aus GPIO18 auf die Sync-Buchse fahren.

Sync-Verbindungen haben gemeinsame Masse; die galvanische Trennung des MIDI-Eingangs am Volca ist davon unabhängig. Für SYNC IN und OUT jeweils eine separate, klar beschriftete Buchse vorsehen. Nicht SYNC OUT mit einem anderen SYNC OUT verbinden.

## ClockSync und Transport

`src/clocksync/ClockSync` ist ein hardwareunabhängiges Modell für `INTERNAL`, `ANALOG_SYNC` und einen reservierten `MIDI_CLOCK`-Modus. Es validiert 2/4 PPQN, unterdrückt Kanten unter 3 ms Abstand, misst das Pulsintervall, erkennt Ausfall nach drei erwarteten Intervallen und berechnet bei 2 PPQN den Zwischenstep. Es wird derzeit nur im Host-Test verwendet. Die bestehende SequencerEngine läuft weiter mit ihrer eigenen internen BPM-Zeitbasis, bis GPIO-Adapter, ISR-Queue und Transportschnittstelle implementiert und am Aufbau geprüft sind.

Für die spätere Integration gilt folgender Vertrag:

- Die GPIO-ISR erfasst mit `esp_timer_get_time()` nur Flankenzeit und Pegel in einer festen Queue. Keine LVGL-Aufrufe, Allokation, Serial-Ausgabe oder I²C im ISR-Kontext. Der Timing-Task verarbeitet die Kanten und validiert Pulsbreite/Abstand. `esp_timer_get_time()` ist laut [Espressif-Dokumentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/esp_timer.html) auch aus ISR-Kontext schnell aufrufbar.
- `INTERNAL`: Play startet Step 1 sofort; BPM bestimmt die Sechzehntel-Zeit. SYNC OUT erzeugt 15-ms-Impulse an jeder oder jeder zweiten Step-Grenze nach konfigurierter Auflösung.
- `ANALOG_SYNC`: Play **armiert** den Sequenzer; erst die nächste gültige Eingangsflanke startet Step 1. Bei 4 PPQN markiert jede Flanke einen Step; bei 2 PPQN liegt ein Zwischenstep bei der halben gemessenen Pulsperiode. Beim allerersten Impuls dient das eingestellte BPM als Startschätzung bis zur zweiten Flanke. Stop sendet sofort Note Off und disarmiert. Eingehende Impulse enthalten **keine** MIDI-Start-/Stop-Nachricht und dürfen gestoppten Transport nicht automatisch starten.
- SYNC OUT reicht im Analogmodus gültige Eingangsimpulse unabhängig vom lokalen Play-Zustand an weitere Volcas weiter. Das ist eine Clock-Weitergabe, kein Transportbefehl. Die Ausgangsstufe benötigt eine kurze, hardwaregetaktete 15-ms-Pulsdauer; UI-Framerate und LVGL dürfen sie nicht bestimmen.
- Bei Ausfall stehen `STOP` (Standard: Note Off, Transport disarmiert), `HOLD` (Note Off, Position halten, am nächsten Impuls fortsetzen) und `FREE_RUN` (nach Ausfall mit zuletzt gemessener Periode weiterlaufen, bei Rückkehr an der nächsten Flanke neu ausrichten) zur Auswahl. Kein stummes Umschalten auf das eingestellte BPM. Diese Policies sind zunächst **Entwurf**, nicht bereits in der Firmware verdrahtet.
- `MIDI_CLOCK` bleibt ein späterer Eingangsadapter mit 24 PPQN. Die SequencerEngine erhält später nur Step-Grenzen und Gate-Zeiten aus einem gemeinsamen Clock-Adapter, keine Kenntnis der Quelle. Ein Puls darf nicht ungeprüft einem MIDI-Step gleichgesetzt werden.

## Nächste Hardwareprüfungen

1. OV5640 abstecken und Flachbandkabel gesichert isolieren; J8 Pin 1/15/17/28 am Rev2.0-Board identifizieren.
2. SYNC-IN-/OUT-Schaltungen stromlos aufbauen und Widerstands-/Kurzschlussprüfung durchführen. GPIO17/18 bleiben bis dahin in der Firmware unangetastet.
3. Mit Oszilloskop oder Logikanalysator Volca-SYNC-OUT-Pegel, Breite, Polarität und Pulsabstand messen. Pegel nach dem Eingangspuffer sowie 5-V-Ausgang unter Last prüfen.
4. Erst danach ISR und Ausgangstimer integrieren; 2/4-PPQN, Tempoänderungen, Stop und drei Ausfallstrategien am realen Aufbau prüfen.
