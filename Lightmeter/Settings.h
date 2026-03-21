#pragma once

#include <stdint.h>
#include "Time.h"

struct Settings {
    void updateEEPROM();
    [[nodiscard]] static Settings load();

    uint16_t baseLogD;
    Time baseTime;
    uint32_t lowResistorDarkVoltageValue;
    uint32_t highResistorDarkVoltageValue;
};

constexpr Settings kDefaultSettings{
    .baseLogD = 400, .baseTime = Time(60), .lowResistorDarkVoltageValue = 80, .highResistorDarkVoltageValue = 947
};
