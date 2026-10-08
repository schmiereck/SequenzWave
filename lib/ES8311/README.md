# ES8311 codec driver

Die drei Quelldateien in `src/` stammen aus der [offiziellen Waveshare-Arduino-Bibliothek](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/tree/283ec84c566c096f8c30493b93dcd4b0bb608de7/Arduino/libraries/es8311). Sie sind hier fest eingecheckt, damit der PlatformIO-Build reproduzierbar bleibt. Der Ursprungscode trägt SPDX `Apache-2.0`; Lizenztext siehe `LICENSE`.

Nur der Codec-Treiber wurde übernommen, keine Beispielarchive, Audiodateien oder SD/Netzwerk-Abhängigkeiten. Aufrufe erfolgen in `src/audio/Speaker.cpp`.
