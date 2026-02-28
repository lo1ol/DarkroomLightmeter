#include "DTEncoder.h"

#include <Arduino.h>

#include "Config.h"
#include "Hardware.h"

void DTEncoder::isr() {
    uint8_t state = (PIND >> 2) & 0x03;
    uint8_t transition = (m_lastEncoderState << 2) | state;

    // clang-format off
    static const int8_t gGrayCodeTable[16] = {
        0, -ENCODER_DIRECTION, ENCODER_DIRECTION, 0,
        ENCODER_DIRECTION, 0, 0, -ENCODER_DIRECTION,
        -ENCODER_DIRECTION, 0, 0, ENCODER_DIRECTION,
        0, ENCODER_DIRECTION, -ENCODER_DIRECTION, 0 };
    //clang-format on

    m_subPos += gGrayCodeTable[transition];
    m_lastEncoderState = state;

    // check we made one click
    if (m_lastEncoderState != 0x03 || m_subPos == 0)
        return;

    int8_t dir = (m_subPos > 0) ? 1 : -1;
    m_subPos = 0;

    uint16_t curTime = millis();
    if (static_cast<uint16_t>(curTime - m_lastTurnChangeTime) < ENCODER_FAST_FAST_TIMEOUT)
        m_turnCounters[2] += dir;
    else if (static_cast<uint16_t>(curTime - m_lastTurnChangeTime) < ENCODER_FAST_TIMEOUT)
        m_turnCounters[1] += dir;
    else
        m_turnCounters[0] += dir;

    m_lastTurnChangeTime = curTime;
}

void DTEncoder::isr_() {
    gEncoder.isr();
}

void DTEncoder::init() {
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN), isr_, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), isr_, CHANGE);
    m_lastEncoderState = (PIND >> 2) & 0x03;
}

int8_t DTEncoder::getShift() const {
    return m_retTurnCounters[0] + m_retTurnCounters[1] + m_retTurnCounters[2];
}

int8_t DTEncoder::getAceleratedShift(uint8_t factor1, uint8_t factor2) const {
    return m_retTurnCounters[0] + m_retTurnCounters[1] * factor1 + m_retTurnCounters[2] * factor2;
}

void DTEncoder::tick() {
    noInterrupts();
    static_assert(sizeof(m_retTurnCounters) == sizeof(m_turnCounters));
    memcpy(m_retTurnCounters, m_turnCounters, sizeof(m_retTurnCounters));
    memset(m_turnCounters, 0, sizeof(m_turnCounters));
    interrupts();
}

bool DTEncoder::getInt(uint16_t& choosen, uint16_t min, uint16_t max, bool ring) const {
    int8_t shift;
    if (max - min > 100)
        shift = getAceleratedShift(3, 6);
    else if (max - min > 30)
        shift = getAceleratedShift(2, 3);
    else
        shift = getShift();

    int16_t res = choosen;

    if (choosen > max) {
        choosen = max;
        return true;
    }

    if (choosen < min) {
        choosen = min;
        return true;
    }

    if (!shift)
        return false;

    if (shift > 1 && shift >= static_cast<int16_t>(max - min))
        shift = (max - min)/2;
    if (shift < -1 && -shift >= static_cast<int16_t>(max - min))
        shift = -(max - min)/2;

    res += shift;

    if (res < static_cast<int16_t>(min))
        res = ring ? (res + max - min) : min;
    else if (res > static_cast<int16_t>(max))
        res = ring ? (res - max + min) : max;

    if (static_cast<uint16_t>(res) == choosen)
        return false;

    choosen = res;
    return true;
}
