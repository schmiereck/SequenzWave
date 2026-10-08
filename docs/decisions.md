# Technische Entscheidungen

## Arduino / PlatformIO als erster Port

Die [offizielle Arduino-Anleitung](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/Arduino) liefert passende Treiber und nennt die verwendeten Bibliotheksversionen. Das Beispiel ist ein Arduino-Sketch, kein fertiges PlatformIO-Projekt. Seine API-Nutzung wird in getrennte C++-Module übertragen. Es gibt für diesen Umfang keinen belegten technischen Zwang zu ESP-IDF.

PlatformIO Espressif32 6.10.0 verwendet Arduino 2.0.17: [Paketmanifest](https://github.com/platformio/platform-espressif32/blob/v6.10.0/platform.json). Der Core wird zusätzlich exakt gepinnt. Das ist eine bewusst konservative Bring-up-Basis, keine Behauptung aktueller Hersteller-Core-Vorgabe oder langfristiger Pflegegarantie. Die Core-Version im grafischen Hersteller-IDE-Screenshot konnte nicht zuverlässig ausgewertet werden. Kompilieren und Linken des gewählten Stacks wurden am 2026-10-08 bestätigt; die Hardwarefunktion bleibt zu prüfen.

Ein Wechsel zu einem Arduino-3.x-fähigen PlatformIO-Paket oder zu ESP-IDF muss bei Problemen separat bewertet werden. Arduino 3.x hat API-Brüche, insbesondere bei Peripherie. Nicht willkürlich Framework-Pakete in eine inkompatible Plattform einsetzen. Für spätere USB-Host-/Timing-Anforderungen kann ESP-IDF sinnvoll sein.

## Speicher und Rendering

Quad-Flash + Octal-PSRAM: `qio_opi`, 16 MB Flash, 8 MB PSRAM. `BOARD_HAS_PSRAM` allein stellt den PSRAM-Busmodus nicht ein. Die Partitionstabelle reserviert zwei 3-MB-App-Slots sowie 0x9F0000 Bytes für späteres LittleFS. OTA und Dateisystem werden noch nicht benutzt.

Ein interner 20-Zeilen-Puffer statt Vollbild benötigt 19.200 Bytes. Die synchrone SPI-Übertragung vermeidet DMA-/PSRAM-Lebensdauerprobleme beim ersten Test. LVGL bekommt RGB565 ohne Byteswap und reale Millisekunden-Differenzen; `LV_TICK_CUSTOM` ist deaktiviert, damit kein doppelter Tick entsteht. Alle LVGL-Aufrufe bleiben in einem Task.

## Kompatibilitätsfallen

- LVGL 9 passt nicht zu den hier verwendeten LVGL-8-Treiberstrukturen.
- GPIO3 ist ein Strap-Pin; keine zusätzlichen Pull-Widerstände für MIDI anschließen.
- LCD-Reset liegt auf dem Expander, nicht auf GPIO1.
- Native USB muss für Serial aktiviert sein; `while (!Serial)` würde Standalone-Start blockieren.
- Displaygröße allein dreht keine Touchkoordinaten.
- Touchfehler erlauben eine sichtbare Diagnose; Fehler an Flash/PSRAM/Expander stoppen die Initialisierung und werden periodisch ausgegeben.
- PSRAM bleibt ein eigener Starttest, obwohl der kleine LVGL-Puffer internen RAM nutzt.
- SPI-Flush blockiert die UI kurzzeitig. Eine spätere MIDI-Engine darf deshalb nicht im UI-Loop getaktet werden.

## Storage, MIDI und Analog-Sync

Auf ausdrücklichen Benutzerwunsch speichert M2 bereits das einzelne Pattern, BPM und Helligkeit als versionierte NVS-Datensätze mit zwei Recovery-Slots und CRC. M3 ergänzt den MIDI-Kanal mit rückwärtskompatibler Formatversion 2. M4 erweitert dies um mehrere Patterns, Kopieren/Löschen und prüft LittleFS für versionierte Pattern-Dateien. Kein Rohdump einer C++-Struktur. MidiOutput hat UART1 und einen getrennten Mock-Diagnosespiegel. Keine CC-Automation und keine Sounddaten im Pattern. Die elektrische DIN-Schaltung ist dokumentiert, aber noch nicht aufgebaut.

Der bestätigte Rev2.0-Header enthält keine zwei völlig unbelegten, gut erreichbaren GPIOs. Für Analog-Sync werden GPIO17/18 aus der nicht benötigten OV5640-Verbindung frei, **erst nachdem das Kamerakabel abgezogen ist**. SD, Audio, USB und MIDI bleiben dadurch verfügbar. Das [ClockSync-Modell](clock_sync.md) ist unabhängig von Arduino/LVGL; Hardware-ISR, 5-V-Ausgang und Transportschnittstelle folgen nach elektrischem Aufbau. Der angeschlossene Lautsprecher bleibt für eine spätere akustische Rückmeldung reserviert, wird jetzt nicht initialisiert.
