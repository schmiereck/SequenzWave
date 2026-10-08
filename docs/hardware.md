# Hardware-Recherche (2026-10-08)

## Variante und Quellenlage

Der Benutzer bestätigt am Etikett **ESP32-S3-Touch-LCD-3.5-C**, FT6336, ST7796, 320×480, 8 MB PSRAM und TCA9554 sowie die Zuordnung zu COM3. Nach Öffnen des Gehäuses hat er die PCB-Revision **Rev2.0** abgelesen und einen angeschlossenen Lautsprecher am Speaker-Port gesehen. Waveshare bezeichnet V1 und V2 als softwarekompatibel. V2 ergänzt unter anderem SD-Erkennung und überarbeitet Stromversorgung/PCB. Nicht mit **3.5B** verwechseln. [Herstellerübersicht](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5)

Bestätigte Familie: ESP32-S3R8, 8 MB PSRAM, 16 MB externer Flash, 320×480 IPS, ST7796 (SPI), FT6336 (I²C). Die 16-MB-Flash-Bauteilbezeichnung unterscheidet sich zwischen den Schaltplänen; deshalb keinen bestimmten Flashhersteller voraussetzen.

## Pins für Meilenstein 1

| Signal | Anschluss | Verwendung |
|---|---|---|
| LCD MOSI | GPIO1 | SPI |
| LCD MISO | GPIO2 | SPI |
| LCD SCLK | GPIO5 | SPI, 40 MHz als Startwert |
| LCD D/C | GPIO3 | Strap-Pin, vorhandene Boardbeschaltung beachten |
| LCD Backlight | GPIO6 | aktiv HIGH; M2: LEDC 20 kHz, 8 Bit, Kanal 0 |
| I²C SDA / SCL | GPIO8 / GPIO7 | geteilter Boardbus, 400 kHz |
| LCD Reset | TCA9554 P1, Adresse 0x20 | kein ESP-GPIO |
| LCD CS | kein softwaregesteuerter GPIO | Referenz benutzt -1 |
| Touch | FT6336, Adresse 0x38 | Polling; kein Interrupt erforderlich |
| USB D− / D+ | GPIO19 / GPIO20 | USB Serial/JTAG CDC |

Maßgebliche ausführbare Referenz: [11_lvgl_arduino_v8](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/blob/main/Arduino/examples/11_lvgl_arduino_v8/11_lvgl_arduino_v8.ino). Die Webseite nennt dieses Beispiel abweichend `11_lvgl_example_v8`. Gelesen wurde `main` am Recherchetag, kein unveränderlicher Hersteller-Commit lokal archiviert.

Der Reset wird über P1 HIGH/LOW/HIGH mit 10/10/200 ms Wartezeit ausgelöst. Arduino_ST7796 übernimmt anschließend Registerinitialisierung, RGB565, Sleep-out und IPS-Inversion. Keine vollständige Hersteller-Firmware und keine Kamera-/Audio-Initialisierung erforderlich. Der Referenzcode initialisiert für diesen Displaytest auch den AXP2101 nicht separat.

## Orientierung

Native Koordinaten: x=0…319, y=0…479. Arduino_GFX Rotation 1 setzt beim ST7796 `MX | MV | BGR`; die logische Fläche ist 480×320. Hieraus abgeleitet wird Touch `(x,y) → (y,319−x)`. Keine zusätzliche LVGL-Rotation und keine SensorLib-Spiegelung aktivieren. Grundlage: [ST7796-Treiber](https://github.com/moononournation/Arduino_GFX/blob/v1.5.5/src/display/Arduino_ST7796.cpp).

Der Host-Test prüft die mathematische Abbildung, nicht die Montageorientierung des realen Touchpanels. Vier Ecken am Board prüfen; etwaige Änderung ausschließlich in TouchTransform.h und mit dokumentiertem Hardwarebefund.

## Reservierungen für spätere Erweiterungen

| Peripherie | GPIOs | Status |
|---|---|---|
| SD_MMC CLK/CMD/D0 | 11/10/9 | nicht aktivieren; eigener Bus |
| I²S MCLK/BCLK/LRCK/SDOUT | 12/13/15/16 | Audio unbenutzt |
| Kamera XCLK/PCLK/VSYNC/HREF | 38/41/17/18 | OV5640 angeschlossen, aber unbenutzt; GPIO17/18 erst nach Abstecken für Sync vorsehen |
| Kamera D0…D7 | 45/47/48/46/42/40/39/21 | nicht als freie MIDI-Pins behandeln |
| Kamera SCCB | 8/7 | gemeinsamer I²C-Bus |
| MIDI UART1 TX | GPIO44, `ESP_RXD`: V1 J8 Pin 27 / V2 J8 Pin 28 | M3: GPIO-Matrix; keine UART0-Bootmeldungen auf diesem Pin |

Quellen: [Audio-Beispiel](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/blob/main/Arduino/examples/01_audio_out/01_audio_out.ino), [SD-Beispiel](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/blob/main/Arduino/examples/07_sd_card_test/07_sd_card_test.ino), [Kamera-Pins](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/Arduino).

M3 verwendet GPIO44 (`ESP_RXD`) als UART1 TX über die GPIO-Matrix. Die grafische Netz-/Headerzuordnung ist in beiden Schaltplänen geprüft: **V1 J8 Pin 27, V2 J8 Pin 28**. Die reale Zugänglichkeit und PCB-Revision im Gehäuse müssen vor der Verdrahtung am Benutzergerät geprüft werden. UART0 kann auf GPIO43 Bootmeldungen ausgeben; deshalb bleibt dieser Pin von MIDI getrennt. Native USB-Diagnose vermeidet Anwendungslogs auf der MIDI-UART. GPIO4 und nicht vollständig zugeordnete Interrupt-/Expanderleitungen werden ausdrücklich nicht als frei angenommen. Schaltung und Bauteile: [Meilenstein 3](milestone3.md).

## Schaltpläne und offene Prüfung

- [V1-Schaltplan](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf)
- [V2-Schaltplan](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5_Rev2.0.pdf)
- [Offizieller Ressourceneinstieg](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/Resources-And-Documents)

Beide PDFs wurden über Textextraktion untersucht. Die Netzanordnung lässt sich daraus nicht überall sicher rekonstruieren; die vollständige visuelle Prüfung bleibt offen. Die implementierten Display-/Touch-Pins sind zusätzlich unmittelbar durch das offizielle Beispiel belegt. USB-C enthält CC-Pulldowns; USB-Host und dessen Versorgung sind kein Bestandteil dieses Meilensteins.

Die DIN-Schaltung und das Pin-Konfliktaudit sind in [Meilenstein 3](milestone3.md) dokumentiert. Keine DIN-Buchse direkt mit ESP-GPIOs verbinden; der vorgesehene Ausgang enthält einen 3,3-V-Puffer und die Widerstände der MIDI-Spezifikation.

Die zusätzliche Analog-Sync-Planung samt Rev2.0-Headerpins, Schutzeingang, 5-V-Ausgangsstufe und Camera/SD/Audio-Konfliktaudit steht in [clock_sync.md](clock_sync.md). Die Sync-GPIOs sind noch nicht initialisiert.
