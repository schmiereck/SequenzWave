# Meilenstein 2: Architektur und Grenzen

## Timing

Ein Step dauert `15.000.000 / BPM` Mikrosekunden (Sechzehntel). Die Engine berechnet jede Grenze aus einem 64-Bit-Epoch und dem Step-Zähler. So summiert sich bei beispielsweise 137 BPM kein Abrundungsfehler pro Step. `esp_timer_get_time()` liefert im Runtime-Adapter die monotone Zeit; der reine C++-Kern erhält sie als Argument.

Der separate Task wacht einmal pro RTOS-Tick auf (im gepinnten Core 1 ms), verarbeitet höchstens acht Befehle und bedient anschließend die Engine. Das ist ein Prototyp mit etwa Millisekunden-Auflösung, keine harte Echtzeitgarantie. SPI-Rendering oder USB-Logging takten keine Noten. Vor DIN-MIDI sind Jitter und UART-Übertragung separat zu messen; bei Bedarf auf hochauflösende Timer-Benachrichtigungen umstellen.

Bei langen Verzögerungen: alte Note lösen, verpasste Steps überspringen, höchstens die noch gültige Note des aktuellen Steps senden. Kein schneller Nachholburst. Ein bereits abgelaufenes Gate erzeugt kein verspätetes Note On. Note Off kommt vor Note On an derselben Grenze. Stop löst die gespeicherte aktive Tonhöhe, auch wenn der zugehörige Step zwischenzeitlich editiert wurde.

Tempoänderungen bearbeiten zunächst fällige Ereignisse, setzen dann das folgende Step-Intervall ab jetzt neu an. Das Gate einer bereits begonnenen Note bleibt erhalten oder endet spätestens an der neuen Step-Grenze. Änderungen von Note/Rest/Gate/Velocity gelten beim nächsten Besuch des Steps. Start bei laufendem Transport ist idempotent; Stop/Start beginnt bei Step 1.

## Task-Grenzen

Die Engine und ihr Pattern haben genau einen Besitzer: den Sequencer-Task. Die UI besitzt eine lokale Bearbeitungskopie. Erst wenn eine Edit-Nachricht in der Queue angenommen wurde, aktualisiert sie diese Kopie. Volle Befehlsqueue: Änderung ablehnen und sichtbare Meldung; nichts stillschweigend als gespeichert anzeigen.

32 Befehle und 128 Mock-Ereignisse passen in statische Queues. Der Snapshot wird in einer kurzen Critical Section kopiert. Keine LVGL-/I²C-/SPI-Aufrufe im Timing-Task, keine Serial-Ausgabe und keine dynamische Allokation dort. Der Task-Stack ist ebenfalls statisch. LVGL und die initialisierten Hardwarebibliotheken dürfen außerhalb des Timingpfads Speicher verwenden.

MockMidiOutput schreibt Ereignisse mit tatsächlichem Aufrufzeitpunkt in die Debugqueue. USB-Ausgabe erfolgt später im Loop. Ohne Monitor werden Records verworfen; bei voller Queue zählt `dropped` verworfene Logs. Das betrifft nur die Diagnose, nicht den Engine-Zustand. Diese Verlustregel ist ausdrücklich nicht für einen späteren echten MidiOutput geeignet.

`late_us` ist die größte beobachtete Verspätung gegenüber einer abgespielten Step-Grenze, nicht eine vollständige Jitterstatistik; `skipped` zählt übersprungene Steps. Beide werden bei Start zurückgesetzt. USB-Ausgabezeitpunkte am PC sind nicht die Notenzeitpunkte.

## Backlight

Bestehender GPIO6, keine Umverdrahtung. LEDC-Kanal 0, Timer 0, 20 kHz und 8 Bit. Kanal/Timer sind für Backlight reserviert; die unbenutzte Kamera darf sie später nicht ungeprüft übernehmen. Der Schieberegler reicht von 2–100 %, Standard ohne gespeicherten Wert 10 %. Prozentwerte sind lineares Tastverhältnis, keine Gamma-Korrektur. Der Benutzer meldet bei 10 % deutlich geringere Wärme; Temperatur und Stromaufnahme wurden nicht gemessen.

Die API wurde gegen `cores/esp32/esp32-hal-ledc.h` und `.c` des lokal gepinnten Arduino-Cores 2.0.17 geprüft. Der Backlight-Pin stammt weiterhin aus dem [offiziellen Waveshare-Beispiel](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/blob/main/Arduino/examples/11_lvgl_arduino_v8/11_lvgl_arduino_v8.ino). Die UI gibt mittels `vTaskDelay` Rechenzeit an Idle ab. Ohne Messung wird keine thermische Verbesserung behauptet.

## Tests

```powershell
g++ -std=c++11 -Wall -Wextra -Werror -Isrc tests/sequencer_engine.cpp src/sequencer/SequencerEngine.cpp -o build/sequencer-test.exe
.\build\sequencer-test.exe
```

Abgedeckt: Start/Stop, doppelte Starts, Rest, Gate-Enden, 100-%-Gate, Note-Off-Reihenfolge, Edit einer aktiven Tonhöhe, Tempoänderungen, ungültige Daten, längere Scheduler-Aussetzer, Pattern-Umbruch, Überschreiten des 32-Bit-Mikrosekundenbereichs und 10.000 Steps bei 137 BPM.

Die Touch-Transformation bleibt unverändert. M2-Bedienabnahme: alle Steps wählen, Noten/Oktaven ändern, Rest, Gate und Velocity prüfen; goldene Laufmarkierung und türkise Auswahl unabhängig testen; Helligkeit 10/40/100 % vergleichen. Stop muss das letzte Note Off erzeugen. Neustart muss den dokumentierten Ausgangszustand wiederherstellen.

## Speichern auf ausdrücklichen Benutzerwunsch

Pattern (16 Steps mit Note, Velocity, Gate, Rest), BPM und Helligkeit liegen in einem 80-Byte-Datensatz mit Formatversion, Generation und CRC32. NVS/Preferences hält zwei abwechselnd geschriebene Slots; beim Boot wird der neueste vollständig gültige Slot gewählt. Bei ungültigen Daten gelten sichere Defaults. Es werden keine Synthesizer-Klangparameter und keine Transportzustände gespeichert.

Touch-Änderungen werden erst nach zwei Sekunden Inaktivität geschrieben, um Flash-Verschleiß beim Schieben und schnellen Editieren zu begrenzen. Die Save-Schaltfläche und Back im Settings-Menü schreiben sofort. UI zeigt Defaults, Saving, Saved oder SAVE ERROR. NVS wird ausschließlich aus dem Arduino-Loop benutzt; die Timing-Aufgabe besitzt weder Filesystem noch Preferences. Die Formatkodierung ist unabhängig vom C++-Struct-Layout und wird auf CRC-Fehler, Bereichsverletzungen und Slot-Fallback getestet.
