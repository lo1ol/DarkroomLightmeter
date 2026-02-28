#pragma once

#include <stdint.h>

#include <ADS1X15.h>

#include "Config.h"

class Lightmeter {
public:
    Lightmeter();
    void init();

    uint16_t getLastMeasure() const;
    void tick();

private:
    void requestNextMeasure();
    int8_t ampAdjust(uint16_t val) const;

    ADS1115 m_ads;
    bool m_readyFlag = false;

    enum AmpLevel : uint8_t {
        R0_G0,
        R0_G1,
        R1_G0,
        R1_G1,
    };

    AmpLevel m_ampLevel = AmpLevel::R0_G0;
    int32_t m_measures[MEASURE_SOFT_CNT] = {};
    uint8_t m_measureIteration = 0;
    bool m_dropNextValue = true;
};
