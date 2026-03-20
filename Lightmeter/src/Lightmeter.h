#pragma once

#include <stdint.h>

#include <ADS1X15.h>

#include "Config.h"

class Lightmeter {
public:
    Lightmeter();
    void poweron();
    void poweroff();

    uint16_t getLastMeasure() const;
    void tick();

    bool calibrate();

private:
    void requestNextMeasure();
    int8_t ampAdjust(uint16_t val) const;

    ADS1115 m_ads;
    volatile bool m_readyFlag = false;

    enum AmpLevel : uint8_t {
        R_LOW_G0,
        R_LOW_G1,
        R_HIGH_G0,
        R_HIGH_G1,
    };

    AmpLevel m_ampLevel = AmpLevel::R_LOW_G0;
    int32_t m_measures[MEASURE_SOFT_CNT] = {};
    uint8_t m_measureIteration = 0;
    bool m_dropNextValue = true;
};
