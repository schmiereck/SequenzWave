# SequenzWave conventions

- Current scope: milestone 1, board bring-up and LVGL test UI only.
- Read README.md and docs/hardware.md before changing board configuration.
- Use PlatformIO, pinned dependencies, Arduino and LVGL 8.4. Never silently upgrade a major version.
- GPIO definitions belong in src/hardware/BoardConfig.h. Confirm changes against the official schematic and examples, with source links.
- Hardware and UI are separate. All LVGL calls run on the Arduino loop task.
- Future SequencerEngine and PatternModel must be plain C++ independent of Arduino/LVGL.
- Future MIDI scheduling must use a separate timing task/clock and bounded command queue, never UI frame timing. No dynamic allocation, logging or filesystem work in the MIDI timing path.
- Future MidiOutput abstracts UART, debug/mock and optional USB transports. Never send synth CC automation; sequence data and sound remain separate.
- Boot delays for device reset are acceptable; sequencer delays are not.
- Native USB CDC is for diagnostics. Do not wait for a connected PC during startup.
- Do not activate camera, audio, SD, Wi-Fi or battery charging configuration without a feature requirement.
- Never call an unexecuted build or hardware test successful. Record exact commands and limits in docs/validation.md.
- Run pio run, pio check and the host touch test after relevant changes. Do not substitute mock API headers for a real firmware build.
- Keep commits focused; no generated binaries, caches, credentials or vendor example archives in Git.
- Project documentation is German; source identifiers are English.
