#pragma once

#include <stdint.h>

class DTEncoder {
public:
    DTEncoder();
    void poweron();
    void poweroff();

    [[nodiscard]] bool getInt(uint16_t& choosen, uint16_t min, uint16_t max, bool ring = false) const;

    bool tick();

private:
    [[nodiscard]] int8_t getShift() const;
    [[nodiscard]] int8_t getAceleratedShift(uint8_t factor1, uint8_t factor2) const;

    static void isr_();
    void isr();

    int8_t m_turnCounters[3] = {};
    int8_t m_lastEncoderState = 0;
    int8_t m_subPos = 0;
    uint16_t m_lastTurnChangeTime = 0;

    int8_t m_retTurnCounters[3] = {};
    bool m_hadEvents = false;
};
