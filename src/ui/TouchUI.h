#pragma once
#include "storage/SettingsCodec.h"
namespace ui {
void create(bool touchAvailable, const storage::Data& loaded);
void refresh();
}
