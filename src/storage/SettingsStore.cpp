#include "SettingsStore.h"
#include <Arduino.h>
#include <Preferences.h>

namespace storage {
namespace {
Preferences preferences;
bool ready = false;
bool pending = false;
bool hasStoredRecord = false;
bool slotA = false;
uint32_t generation = 0;
uint32_t editedAt = 0;
SaveStatus saveStatus = SaveStatus::Defaults;
Data waiting;

Record readSlot(const char* key) {
    Record record{};
    if (preferences.getBytesLength(key) == kRecordSize) {
        preferences.getBytes(key, record.data(), record.size());
    }
    return record;
}
}  // namespace

bool begin(Data& loaded) {
    loaded = Data{};
    ready = preferences.begin("sequenz", false);
    if (!ready) {
        saveStatus = SaveStatus::Error;
        return false;
    }
    const Record a = readSlot("slotA");
    const Record b = readSlot("slotB");
    hasStoredRecord = newest(a, b, loaded, generation, slotA);
    saveStatus = hasStoredRecord ? SaveStatus::Saved : SaveStatus::Defaults;
    return true;
}

void schedule(const Data& data) {
    waiting = data;
    pending = true;
    editedAt = millis();
    saveStatus = ready ? SaveStatus::Pending : SaveStatus::Error;
}

bool saveNow() {
    if (!ready) { saveStatus = SaveStatus::Error; return false; }
    if (!pending) return true;
    const Record record = encode(waiting, generation + 1);
    const char* target = slotA ? "slotB" : "slotA";
    if (preferences.putBytes(target, record.data(), record.size()) == record.size()) {
        generation++;
        slotA = !slotA;
        pending = false;
        saveStatus = SaveStatus::Saved;
        return true;
    } else {
        saveStatus = SaveStatus::Error;
        editedAt = millis(); // Retry after two seconds without blocking the UI.
        return false;
    }
}
void service(uint32_t nowMs) {
    if (pending && nowMs - editedAt >= 2000) saveNow();
}
SaveStatus status() { return saveStatus; }
bool loadedFromNvs() { return hasStoredRecord; }
}  // namespace storage
