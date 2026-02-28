#pragma once

#include <stdint.h>
#include "Utils/Time.h"

struct Settings {
    void updateEEPROM();
    [[nodiscard]] static Settings load();

    uint16_t baseLogD;
    Time baseTime;
};

constexpr Settings kDefaultSettings{ .baseLogD = 400, .baseTime = Time(60) };
