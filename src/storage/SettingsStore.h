#pragma once
#include "SettingsCodec.h"

namespace storage {
enum class SaveStatus : uint8_t { Defaults, Pending, Saved, Error };
// UI task only. Opens NVS and loads the newest valid slot; defaults on failure.
bool begin(Data& loaded);
void schedule(const Data& data);
bool saveNow();
void service(uint32_t nowMs);
SaveStatus status();
bool loadedFromNvs();
}  // namespace storage
