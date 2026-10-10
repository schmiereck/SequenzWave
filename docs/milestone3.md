# Meilenstein 3: DIN-MIDI OUT und analoger Volca-Sync

Dieses Dokument ist der gemeinsame Verdrahtungsplan für **MIDI OUT, SYNC IN und SYNC OUT** am ESP32-S3-Touch-LCD-3.5-C **Rev2.0**. Die OV5640-Kamera ist ausgebaut. MIDI-UART ist bereits in der Firmware aktiv; die beiden Sync-GPIOs sind bisher **nur reserviert**, nicht initialisiert. Die Sync-Schaltungen können elektrisch aufgebaut und geprüft werden. Der Sequenzer wertet Eingangsimpulse aber erst nach der späteren [ClockSync-Integration](clock_sync.md) aus und erzeugt vorher keine Ausgangsimpulse.

## DIN-MIDI OUT: Anschluss am Waveshare-Board

Der Benutzer hat die Platine im Gehäuse als **Rev2.0** identifiziert. Die [V1-](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf) und [V2-Schaltpläne](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5_Rev2.0.pdf) zeigen `ESP_RXD = GPIO44` am 2×16-Header J8: V1 Pin 27, **bei diesem Rev2.0-Board Pin 28**. Der GPIO ist im Projekt **UART1 TX** über die GPIO-Matrix, obwohl er als UART0 RX beschriftet ist. Das vermeidet UART0-Bootmeldungen auf GPIO43. GPIO44 hat laut Schaltplan keine Display-, Touch-, Kamera-, SD- oder Audiobelegung. `ESP_TXD`/GPIO43 auf J8 Pin 26 bleibt unbenutzt. 3,3 V liegt auf J8 Pin 31/32, GND auf Pin 29/30. Vor dem Anschluss die Ausrichtung des realen Headers und die Pin-1-Markierung prüfen.

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

Die Buchse von der **Lötseite anhand ihrer eingeprägten Pinnummern** prüfen; Spiegelung ist ein häufiger Verdrahtungsfehler. Nur den MIDI-OUT-Anschluss mit dem MIDI-IN des Volca FM verbinden. USB bleibt für Strom und Diagnose verfügbar. 5 V dürfen nicht an den 3,3-V-Header geführt werden.

## Analog-Sync: Anschlüsse und Schaltung

Der [Rev2.0-Schaltplan](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5_Rev2.0.pdf) führt `CAM_VSYNC`/GPIO17 auf **J8 Pin 15** und `CAM_HREF`/GPIO18 auf **J8 Pin 17**. Beide Leitungen sind erst durch die ausgebaute Kamera für Sync verfügbar. **J8 Pin 1 ist USB-VBUS (5 V), Pin 2 ist VBAT und darf hier nicht verwendet werden.** GND liegt auf Pin 29/30 und 3V3 auf Pin 31/32. Pin 28 bleibt MIDI OUT. Pin-1-Markierung und Zählrichtung am echten Header vor dem Löten prüfen.

Der [Volca-FM-Herstellerleitfaden](https://cdn.korg.com/us/support/download/files/f160e38a8112b463f6546dad35091bd5.pdf) spezifiziert 3,5-mm-Mono-Sync-Buchsen, 5-V-Ausgangsimpulse von etwa 15 ms und einen maximalen SYNC-IN-Pegel von 20 V. `SyncStp 1` bedeutet ein Impuls je Volca-Step (4 PPQN bei Sechzehnteln); `SyncStp 2` ein Impuls je zwei Steps (2 PPQN, Werkseinstellung). Für die erste Abnahme gleiche Auflösung und steigende Polarität an allen Volcas einstellen. Der analoge Sync trägt **keinen separaten Start-/Stop-Befehl**; das Transportverhalten steht in [clock_sync.md](clock_sync.md).

### SYNC IN: 5-V-Impulse geschützt auf GPIO17

Eine eigene 3,5-mm-**Mono-Eingangsbuchse** bekommt Spitze = Signal und Schaft = gemeinsame Masse. Die Spannungsteilung 22 kΩ / 33 kΩ ergibt aus 5 V ungefähr 3,0 V. 1 nF filtert kurze Störungen; eine 3,6-V-Zenerdiode begrenzt Fehlpegel. Der nachgeschaltete [SN74LVC1G17-Schmitt-Puffer](https://www.ti.com/lit/ds/symlink/sn74lvc1g17.pdf) läuft an 3,3 V, akzeptiert bis 5,5 V am Eingang und liefert am ESP nur 3,3-V-Logik. Die Zenerdiode ist **kein Ersatz** für den Serienwiderstand.

```text
Volca SYNC OUT ── Mono-Stecker ── SYNC-IN-Buchse Spitze ── 22 kΩ ──●── U2 A
                                                                │
                                                                ├── 33 kΩ ── GND
                                                                ├── 1 nF ─── GND
                                                                └── Zener 3,6 V ── GND
                                                                    Kathode am ●,
                                                                    Anode an GND
SYNC-IN-Buchse Schaft ───────────────────────────────────────────────── GND
U2 SN74LVC1G17: VCC ── J8 31/32 (3V3); GND ── J8 29/30;
                   Y ── J8 15 (GPIO17); 100 nF direkt VCC–GND; NC offen.
```

### SYNC OUT: GPIO18 auf 5-V-Volca-Pegel

Eine zweite, klar als **SYNC OUT** beschriftete 3,5-mm-Mono-Buchse erhält Spitze = Ausgang und Schaft = GND. Der [SN74AHCT1G125](https://www.ti.com/lit/ds/symlink/sn74ahct1g125.pdf) wird aus **USB-VBUS 5 V** versorgt; sein TTL-Eingang erkennt 3,3 V sicher als HIGH. Ein 10-kΩ-Pulldown hält den Ausgang während Reset/Boot LOW. Der 1-kΩ-Ausgangswiderstand begrenzt Strom bei versehentlichem Kurzschluss. `/OE` ist aktiv LOW und wird mit GND verbunden. **SYNC OUT funktioniert mit dieser Schaltung nur, wenn USB-VBUS vorhanden ist**; Batteriebetrieb ohne USB benötigt später eine eigene 5-V-Stufe.

```text
J8 17 (GPIO18) ──●── U3 A                 U3 Y ── 1 kΩ ── SYNC-OUT-Buchse Spitze
                 │
                 └── 10 kΩ ── GND
U3 SN74AHCT1G125: VCC ── J8 1 (VBUS/5 V); GND ── J8 29/30;
                    /OE ── GND; 100 nF direkt VCC–GND.
SYNC-OUT-Buchse Schaft ─────────────────────────────────── GND
```

**Nicht** GPIO18 direkt mit einer Sync-Buchse oder 5 V verbinden. SYNC OUT nur an den **SYNC IN** des nächsten Volca anschließen, niemals Ausgang mit Ausgang. Bei externem Sync soll die spätere Firmware gültige Eingangsimpulse auch bei gestopptem lokalem Transport weitergeben; bei internem BPM wird sie eigene etwa 15-ms-Impulse erzeugen. Die Ausgabe läuft über einen Timer, unabhängig von LVGL oder Display-Framerate.

### IC-Pins und gemeinsame Bauteilliste

Die folgenden Pinzahlen gelten für die **5-polige SOT-23/DBV-Variante in Draufsicht auf das IC**. Bei anderer Gehäusevariante die konkrete Bestellnummer und das Datenblatt prüfen; der Adapter führt die Pins nicht automatisch in dieser Reihenfolge heraus.

| IC | Pin 1 | Pin 2 | Pin 3 | Pin 4 | Pin 5 |
|---|---|---|---|---|---|
| U1 [SN74LVC1G125](https://www.ti.com/lit/ds/symlink/sn74lvc1g125.pdf), MIDI | `/OE` → GND | A ← GPIO44 | GND | Y → 10 Ω → DIN 5 | 3V3 |
| U2 [SN74LVC1G17](https://www.ti.com/lit/ds/symlink/sn74lvc1g17.pdf), SYNC IN | NC, offen | A ← Teilerknoten | GND | Y → GPIO17 | 3V3 |
| U3 [SN74AHCT1G125](https://www.ti.com/lit/ds/symlink/sn74ahct1g125.pdf), SYNC OUT | `/OE` → GND | A ← GPIO18 | GND | Y → 1 kΩ → Spitze | **VBUS 5 V** |

| Bauteil | Anzahl | Einsatz |
|---|---:|---|
| 5-polige 180°-DIN-Einbaubuchse | 1 | MIDI OUT |
| 3,5-mm-Mono-Einbaubuchse | 2 | SYNC IN und SYNC OUT, getrennt beschriften |
| SN74LVC1G125, SN74LVC1G17, SN74AHCT1G125 | je 1 | MIDI-Puffer, Sync-Eingang, Sync-Ausgang |
| 33 Ω / ≥0,5 W und 10 Ω / ≥0,25 W | je 1 | MIDI-Stromschleife |
| 10 kΩ | 2 | MIDI-Idle-Pullup und SYNC-OUT-Pulldown |
| 22 kΩ und 33 kΩ | je 1 | SYNC-IN-Spannungsteiler |
| 1 kΩ | 1 | SYNC-OUT-Ausgangsbegrenzung |
| 3,6-V-Zenerdiode, ≥0,25 W | 1 | SYNC-IN-Überspannungsbegrenzung |
| 1 nF | 1 | SYNC-IN-Störfilter |
| 100 nF | 3 | je einer direkt an U1/U2/U3 zwischen VCC und GND |
| SOT-23-5-Adapter, Trägerplatine, Litzen, 3,5-mm-Monokabel | nach Aufbau | Anschluss und Montage |

Falls die Bestellung bisher nur MIDI OUT aus diesem Dokument abdeckte, kommen für Sync **zwei Mono-Buchsen, U2 und U3, je ein 22-kΩ-, 33-kΩ- und 1-kΩ-Widerstand, ein weiterer 10-kΩ-Widerstand, zwei weitere 100-nF-Kondensatoren, 1 nF, eine 3,6-V-Zenerdiode sowie passende Adapter/Kabel** hinzu.

Die Klinkenbuchsen-Kontakte mit einem Durchgangsprüfer bei eingestecktem Kabel als **Spitze/Schaft** identifizieren. Insbesondere Buchsen mit zusätzlichen Schaltkontakten dürfen nicht nach bloßer Position der Lötfahnen verdrahtet werden. Alle drei Schaltungen teilen 3V3/GND beziehungsweise VBUS/GND des J8; **VBAT Pin 2 bleibt frei**.

## Aufbau und Inbetriebnahme

1. Board stromlos lassen, J8 Pin 1/15/17/28/29–32 identifizieren und den Kameraanschluss frei lassen. Buchsen, Widerstände, IC-Adapter und Abblockkondensatoren verdrahten; Polarität der Zenerdiode prüfen. Durchgang und fehlende Kurzschlüsse zwischen VBUS, 3V3, GPIOs und GND messen.
2. Zunächst USB-Versorgung **ohne Volca-Kabel** einschalten: an U2/U1 3,3 V, an U3 bei USB-Versorgung ungefähr 5 V; SYNC OUT im Idle LOW. Einen externen 5-V-Testimpuls mit **gemeinsamer Masse** an SYNC IN anlegen und Teilerknoten (ungefähr 3,0 V) sowie U2 Y/GPIO17 (0–3,3 V) messen. Keine externe 5-V-Leitung direkt auf GPIO17/18 legen.
3. MIDI OUT getrennt nach obigem Aufbau prüfen. Ein SYNC-OUT-Puls lässt sich erst nach Implementierung des noch ausstehenden GPIO-/Timer-Moduls erwarten; **die jetzige Firmware erzeugt keinen**. Danach Polarität, Pulsbreite, Pegel unter Last und 2/4 PPQN mit Logikanalysator/Oszilloskop und anschließend mit den Volcas prüfen. Erst nach diesem elektrischen Test SYNC OUT an einen Volca SYNC IN anschließen.

## Software

`UartMidiOutput` kodiert Note On/Off als je drei MIDI-Bytes. Die feste 96-Byte-Ringqueue wird nur vom Sequencer-Task benutzt. `uart_tx_chars` füllt ausschließlich freien Platz im Hardware-FIFO und wartet nicht auf dessen Leerung; der Task versucht das Entleeren alle 1 ms erneut. Anwendungs-USB-Logs laufen weiterhin getrennt im UI-Loop. `midi_drop` zählt volle MIDI-Ringqueues; bei einem Wert ungleich null ist die Ausgabe nicht verlässlich und die Ursache muss vor einem Bühneneinsatz behoben werden.

Der MIDI-Kanal 1–16 ist im Settings-Bildschirm einstellbar. Ein Wechsel während einer klingenden Note sendet zuerst Note Off auf dem bisherigen Kanal; die nächste Note kommt auf dem neuen Kanal. Der Kanal wird mit Pattern/BPM und den übrigen Einstellungen im versionierten NVS-Datensatz gespeichert; alte Datensätze bleiben lesbar.

Der Mock-Ausgang bleibt als Diagnose-Spiegel aktiv. Ohne angeschlossene DIN-Hardware kann die UART-Konfiguration und das MIDI-Byteformat gebaut und getestet werden; echte Stromschleife, Pegel, Byte-Timing und Klang am Volca sind damit noch **nicht** verifiziert. Ein Logikanalysator am Puffer-Eingang beziehungsweise an DIN Pin 5 und ein Volca-Test sind die nächsten Hardwareprüfungen. Die analoge Sync-Implementierung ist noch nicht mit dem Sequencer verbunden; [clock_sync.md](clock_sync.md) beschreibt den geplanten Takt- und Transportvertrag.
