#pragma once

#include <GyverSegment.h>

#include "Config.h"
#include "Settings.h"
#include "Utils/Time.h"

class Display {
public:
    Display();

    void poweron();
    void poweroff();

    void tick();

    enum class ShowValMode {
        Normal,
        Set,
    };

    void showVal(uint16_t, ShowValMode = ShowValMode::Normal);
    void showRelVal(int16_t);

    enum class ShowTimeMode {
        Normal,
        SetMin,
        SetSec,
    };
    void showTime(Time, ShowTimeMode = ShowTimeMode::Normal);

    void resetBlink(bool showBlinked = true);

private:
    void tickVal();
    void tickRelVal();
    void tickTime();

    enum class ShowMode {
        Val,
        RelVal,
        Time,
    };

    union ShowingVal {
        struct {
            int16_t val;
        } relVal;
        struct {
            uint16_t val;
            ShowValMode mode;
        } val;
        struct {
            Time val;
            ShowTimeMode mode;
        } time;
    };

    ShowMode m_showMode;
    ShowingVal m_val{};

    bool m_needUpdate = false;
    bool m_showBlinked;
    uint16_t m_prevBlinkTime;
    bool m_blinking = false;

    Disp1637Colon m_display;
};
