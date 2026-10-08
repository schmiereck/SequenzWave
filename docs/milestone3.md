# Meilenstein 3: DIN-MIDI OUT

## Anschluss am Waveshare-Board

Für die bestätigte Produktfamilie ESP32-S3-Touch-LCD-3.5-C zeigen die [V1-](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf) und [V2-Schaltpläne](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5_Rev2.0.pdf) `ESP_RXD = GPIO44` am 2×16-Header J8. **Die Pinposition unterscheidet sich: V1 = Pin 27, V2 = Pin 28.** Dieser GPIO ist im Projekt **UART1 TX** über die GPIO-Matrix, obwohl er als UART0 RX beschriftet ist. Das vermeidet UART0-Bootmeldungen auf GPIO43. GPIO44 hat laut beiden Schaltplänen keine Display-, Touch-, Kamera-, SD- oder Audiobelegung. Der `ESP_TXD`-Pin (GPIO43) bleibt unbenutzt: V1 J8 Pin 25, V2 J8 Pin 26. 3,3 V liegt auf J8 Pin 31/32, GND auf Pin 29/30. Die PCB-Revision und Zugänglichkeit des Headers im Gehäuse sind am konkreten Exemplar vor der Verdrahtung zu prüfen; **keine Verdrahtung nur nach der Pinzahl ohne Revisionskontrolle**.

Die Ausgangsschaltung folgt [MIDI CA-033, Bild 1](https://www.midi.org/wp-content/uploads/wpforo/default_attachments/1709416667-ca33-MIDI-10-Electrical-Specification-Update.pdf): 31.250 Baud, 8N1, 3,3-V-Variante. Ein **nicht invertierender 3,3-V-Logikpuffer** zwischen GPIO44 und dem 10-Ω-Widerstand entlastet den ESP-Pin. Der Puffer muss etwa 5 mA sicher treiben können; ein geeigneter Kandidat ist der [SN74LVC1G125](https://www.ti.com/lit/ds/symlink/sn74lvc1g125.pdf) mit ±24-mA-Ausgangstreiber bei 3,3 V. Die Bauteilvariante ist klein und benötigt für einen Steckbrettaufbau einen passenden Adapter; die genaue Pinbelegung vor dem Drahtaufbau anhand des Datenblatts prüfen. Ein direkter ESP-GPIO-Anschluss ist für diesen Aufbau nicht vorgesehen.

```text
J8 Pin 31/32, 3V3 ── 33 Ω, 5 %, ≥0,5 W ── DIN MIDI OUT Pin 4
J8 GPIO44/ESP_RXD: V1 Pin 27, V2 Pin 28 ── Puffer Eingang
                       Puffer Ausgang ── 10 Ω, 5 %, ≥0,25 W ── DIN MIDI OUT Pin 5
J8 Pin 29/30, GND ── Puffer GND ────────── DIN MIDI OUT Pin 2
3V3 ── Puffer VCC; 100 nF direkt zwischen Puffer VCC und GND
3V3 ── 10 kΩ ── Puffer Eingang (definierter Idle-Pegel beim Boot)
Puffer /OE aktivieren gemäß Datenblatt, typischerweise an GND.
DIN Pins 1 und 3: unbeschaltet.
```

Benötigt werden eine 5-polige 180°-DIN-Buchse, ein nicht invertierender 3,3-V-Puffer, Widerstände 33 Ω/0,5 W, 10 Ω/0,25 W und 10 kΩ, ein 100-nF-Abblockkondensator sowie geeignete Leitungen/Trägerplatine. Die Buchse von der **Lötseite anhand ihrer eingeprägten Pinnummern** prüfen; Spiegelung ist ein häufiger Verdrahtungsfehler. Nur den MIDI-OUT-Anschluss mit dem MIDI-IN des Volca FM verbinden. USB bleibt für Strom und Diagnose verfügbar. 5 V dürfen nicht an den 3,3-V-Header geführt werden.

## Software

`UartMidiOutput` kodiert Note On/Off als je drei MIDI-Bytes. Die feste 96-Byte-Ringqueue wird nur vom Sequencer-Task benutzt. `uart_tx_chars` füllt ausschließlich freien Platz im Hardware-FIFO und wartet nicht auf dessen Leerung; der Task versucht das Entleeren alle 1 ms erneut. Anwendungs-USB-Logs laufen weiterhin getrennt im UI-Loop. `midi_drop` zählt volle MIDI-Ringqueues; bei einem Wert ungleich null ist die Ausgabe nicht verlässlich und die Ursache muss vor einem Bühneneinsatz behoben werden.

Der MIDI-Kanal 1–16 ist im Settings-Bildschirm einstellbar. Ein Wechsel während einer klingenden Note sendet zuerst Note Off auf dem bisherigen Kanal; die nächste Note kommt auf dem neuen Kanal. Der Kanal wird mit Pattern/BPM/Helligkeit im bestehenden NVS-Datensatz gespeichert. Formatversion 2 nutzt zuvor freie Bits des BPM-High-Bytes; Version-1-Datensätze werden weiterhin geladen und erhalten Kanal 1.

Der Mock-Ausgang bleibt als Diagnose-Spiegel aktiv. Ohne angeschlossene DIN-Hardware kann die UART-Konfiguration und das MIDI-Byteformat gebaut und getestet werden; echte Stromschleife, Pegel, Byte-Timing und Klang am Volca sind damit noch **nicht** verifiziert. Ein Logikanalysator am Puffer-Eingang bzw. an DIN Pin 5 und ein Volca-Test sind die nächsten Hardwareprüfungen.
