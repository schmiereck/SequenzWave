# SequenzWave conventions

- Current scope: milestone 2, 16-step sequencer with mock MIDI, adjustable brightness and user-requested settings persistence. No DIN output yet.
- Read README.md and docs/hardware.md before changing board configuration.
- Use PlatformIO, pinned dependencies, Arduino and LVGL 8.4. Never silently upgrade a major version.
- GPIO definitions belong in src/hardware/BoardConfig.h. Confirm changes against the official schematic and examples, with source links.
- Hardware and UI are separate. All LVGL calls run on the Arduino loop task.
- SequencerEngine and PatternModel are plain C++ independent of Arduino/LVGL; keep host tests covering scheduling and note lifetimes.
- MIDI scheduling uses a separate timing task/clock and bounded command queue, never UI frame timing. No dynamic allocation, logging or filesystem work in the MIDI timing path.
- Future MidiOutput abstracts UART, debug/mock and optional USB transports. Never send synth CC automation; sequence data and sound remain separate.
- Boot delays for device reset are acceptable; sequencer delays are not.
- Native USB CDC is for diagnostics. Do not wait for a connected PC during startup.
- Storage writes run only in the UI loop after a debounce; keep the NVS format versioned with checksum and recovery slot.
- Do not activate camera, audio, SD, Wi-Fi or battery charging configuration without a feature requirement.
- Never call an unexecuted build or hardware test successful. Record exact commands and limits in docs/validation.md.
- Run pio run, pio check and the host touch test after relevant changes. Do not substitute mock API headers for a real firmware build.
- Keep commits focused; no generated binaries, caches, credentials or vendor example archives in Git.
- Project documentation is German; source identifiers are English.
