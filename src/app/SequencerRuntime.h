#pragma once
#include "sequencer/SequencerEngine.h"

namespace app {
enum class Action : uint8_t { Toggle, Start, Stop, Tempo, Edit };
struct Command {
    Action action = Action::Stop;
    uint16_t value = 0;
    sequencer::Step step;
};
struct Snapshot {
    sequencer::Status transport;
    uint32_t droppedLogs = 0;
};
bool begin(const sequencer::Pattern& pattern, uint16_t bpm);
bool send(const Command& command);
Snapshot snapshot();
// Called only by the UI task; USB I/O never runs in the timing task.
void serviceDebug();
}  // namespace app
