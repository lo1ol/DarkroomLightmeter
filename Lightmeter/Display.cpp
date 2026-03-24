#include "Display.h"

#include <Arduino.h>

Display::Display() : m_display(DISPLAY_DIO_PIN, DISPLAY_CLK_PIN, true) {
    pinMode(DISPLAY_POWER_PIN, OUTPUT);
}

void Display::poweron() {
    m_display.buffer[0] = 0;
    m_display.buffer[1] = 0;
    m_display.buffer[2] = 0;
    m_display.buffer[3] = 0;

    digitalWrite(DISPLAY_POWER_PIN, HIGH);
    m_display.update();
    m_display.power(true);

    m_display.brightness(0);
    m_needUpdate = false;
    m_blinking = false;
}

void Display::poweroff() {
    m_display.buffer[0] = 0;
    m_display.buffer[1] = 0;
    m_display.buffer[2] = 0;
    m_display.buffer[3] = 0;
    m_display.update();

    m_display.power(false);
    digitalWrite(DISPLAY_POWER_PIN, LOW);
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

uint8_t getCharCode(char ch) {
    switch (ch) {
    case '0':
        return 0x3F;
    case '1':
        return 0x30;
    case '2':
        return 0x5B;
    case '3':
        return 0x79;
    case '4':
        return 0x74;
    case '5':
        return 0x6D;
    case '6':
        return 0x6F;
    case '7':
        return 0x38;
    case '8':
        return 0x7F;
    case '9':
        return 0x7D;
    case 'C':
        return 0x0F;
    case 'A':
        return 0x7E;
    case 'L':
        return 0x07;
    case '-':
        return 0x40;
    case 'r':
        return 0x42;
    case 'g':
        return 0x7D;
    case 'o':
        return 0x63;
    case 'b':
        return 0x67;
    case 'd':
        return 0x73;
    default:
        return 0x00;
    }
};

inline uint8_t dig(uint16_t val, uint16_t pos) {
    if (val < pos)
        return 0;

    return getCharCode('0' + ((val / pos) % 10));
}

} // namespace

void Display::tickVal() {
    if (m_val.val.mode == ShowValMode::Set && m_showBlinked)
        return;

    uint16_t val = m_val.val.val;

    if (val == 999) {
        m_display.buffer[1] = getCharCode('L');
        m_display.buffer[0] = getCharCode('0');
    } else {
        m_display.buffer[3] = dig(val, 1000);
        m_display.buffer[2] = dig(val, 100);
        m_display.buffer[1] = dig(val, 10);
        m_display.buffer[0] = dig(val, 1);

        if (val == 0)
            m_display.buffer[0] = getCharCode('0');
    }

    m_display.colon(false);
}

void Display::tickRelVal() {
    int16_t val = m_val.relVal.val;

    bool minus = val < 0;

    if (minus)
        val = -val;

    m_display.buffer[3] = 0;
    m_display.buffer[2] = dig(val, 100);
    m_display.buffer[1] = dig(val, 10);
    m_display.buffer[0] = dig(val, 1);

    if (val == 0)
        m_display.buffer[0] = getCharCode('0');

    uint8_t signPos;
    if (val < 10)
        signPos = 1;
    else if (val < 100)
        signPos = 2;
    else
        signPos = 3;

    m_display.buffer[signPos] = getCharCode(minus ? '-' : 'r');

    m_display.colon(false);
}

void Display::tickTime() {
    Time t = m_val.time.val;

    if (m_val.time.mode != ShowTimeMode::SetMin || !m_showBlinked) {
        uint8_t mins = min(t.mins(), 99);
        m_display.buffer[3] = getCharCode('0' + mins / 10);
        m_display.buffer[2] = getCharCode('0' + mins % 10);
    }

    if (m_val.time.mode != ShowTimeMode::SetSec || !m_showBlinked) {
        uint8_t secs = min(t.secs(), 59);
        m_display.buffer[1] = getCharCode('0' + secs / 10);
        m_display.buffer[0] = getCharCode('0' + secs % 10);
    }

    m_display.colon(true);
}

void Display::showCalibrationAnim() {
    m_display.buffer[3] = 0;
    m_display.buffer[2] = getCharCode('C');
    m_display.buffer[1] = getCharCode('A');
    m_display.buffer[0] = getCharCode('L');
    m_display.update();
    delay(2000);

    m_display.buffer[0] = 0;
    m_display.buffer[2] = 0;
    m_display.buffer[1] = 0;

    m_display.buffer[0] = getCharCode('3');
    m_display.update();
    delay(1000);

    m_display.buffer[0] = getCharCode('2');
    m_display.update();
    delay(1000);

    m_display.buffer[0] = getCharCode('1');
    m_display.update();
    delay(1000);

    m_display.buffer[0] = 0;
    m_display.update();
}

void Display::showRes(bool good) {
    if (good) {
        m_display.buffer[3] = getCharCode('g');
        m_display.buffer[2] = getCharCode('0');
        m_display.buffer[1] = getCharCode('0');
        m_display.buffer[0] = getCharCode('d');
    } else {
        m_display.buffer[3] = 0;
        m_display.buffer[2] = getCharCode('b');
        m_display.buffer[1] = getCharCode('A');
        m_display.buffer[0] = getCharCode('d');
    }

    m_display.update();
    delay(2000);
}
