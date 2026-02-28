#include "Display.h"

#include <Arduino.h>

Display::Display() : m_display(DISPLAY_DIO_PIN, DISPLAY_CLK_PIN, true) {
    m_display.brightness(0);
}

void Display::showVal(uint16_t val, ShowValMode mode) {
    m_showMode = ShowMode::Val;
    m_val.val.val = val;
    m_val.val.mode = mode;
    m_blinking = mode != ShowValMode::Normal;
    resetBlink(false);
}

void Display::showRelVal(int16_t val) {
    m_showMode = ShowMode::RelVal;
    m_val.relVal.val = val;
    m_blinking = false;
    m_needUpdate = true;
}

void Display::showTime(Time val, ShowTimeMode mode) {
    m_showMode = ShowMode::Time;
    m_val.time.val = val;
    m_val.time.mode = mode;
    m_blinking = mode != ShowTimeMode::Normal;
    resetBlink(false);
}

void Display::resetBlink(bool showBlinked) {
    m_showBlinked = showBlinked;
    m_prevBlinkTime = millis();
    m_needUpdate = true;
}

void Display::tick() {
    if (m_blinking && static_cast<uint16_t>(millis() - m_prevBlinkTime) > 500)
        resetBlink(!m_showBlinked);

    if (!m_needUpdate)
        return;

    m_needUpdate = false;

    m_display.buffer[0] = 0;
    m_display.buffer[1] = 0;
    m_display.buffer[2] = 0;
    m_display.buffer[3] = 0;

    switch (m_showMode) {
    case ShowMode::Val:
        tickVal();
        break;
    case ShowMode::Time:
        tickTime();
        break;
    case ShowMode::RelVal:
        tickRelVal();
        break;
    };

    m_display.update();
}

namespace {
inline uint8_t dig(uint16_t val, uint16_t pos) {
    if (val < pos)
        return 0;

    return sseg::getCharCode('0' + ((val / pos) % 10));
}

} // namespace

void Display::tickVal() {
    if (m_val.val.mode == ShowValMode::Set && m_showBlinked)
        return;

    uint16_t val = m_val.val.val;

    m_display.buffer[0] = dig(val, 1000);
    m_display.buffer[1] = dig(val, 100);
    m_display.buffer[2] = dig(val, 10);
    m_display.buffer[3] = dig(val, 1);

    if (val == 0)
        m_display.buffer[3] = sseg::getCharCode('0');

    m_display.colon(false);
}

void Display::tickRelVal() {
    int16_t val = m_val.relVal.val;

    bool minus = val < 0;

    if (minus)
        val = -val;

    m_display.buffer[0] = 0;
    m_display.buffer[1] = dig(val, 100);
    m_display.buffer[2] = dig(val, 10);
    m_display.buffer[3] = dig(val, 1);

    if (val == 0)
        m_display.buffer[3] = sseg::getCharCode('0');

    uint8_t signPos;
    if (val < 10)
        signPos = 2;
    else if (val < 100)
        signPos = 1;
    else
        signPos = 0;

    m_display.buffer[signPos] = minus ? 64 : 80;

    m_display.colon(false);
}

void Display::tickTime() {
    Time t = m_val.time.val;

    if (m_val.time.mode != ShowTimeMode::SetMin || !m_showBlinked) {
        uint8_t mins = min(t.mins(), 99);
        m_display.buffer[0] = sseg::getCharCode('0' + mins / 10);
        m_display.buffer[1] = sseg::getCharCode('0' + mins % 10);
    }

    if (m_val.time.mode != ShowTimeMode::SetSec || !m_showBlinked) {
        uint8_t secs = min(t.secs(), 59);
        m_display.buffer[2] = sseg::getCharCode('0' + secs / 10);
        m_display.buffer[3] = sseg::getCharCode('0' + secs % 10);
    }

    m_display.colon(true);
}
