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
    enum AmpLevel : uint8_t {
        R_LOW_G0,
        R_LOW_G1,
        R_HIGH_G0,
        R_HIGH_G1,
    };

    void requestNextMeasure(AmpLevel, bool forceSetAmp = false);
    int8_t ampAdjust(int16_t val) const;

    int16_t syncGetVal(AmpLevel);

    ADS1115 m_ads;
    volatile bool m_readyFlag;

    AmpLevel m_ampLevel = R_HIGH_G1;
    int32_t m_measures[MEASURE_SOFT_CNT] = {};
    uint8_t m_measureIteration = 0;
    bool m_dropNextValue;
};
