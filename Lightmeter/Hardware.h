#pragma once

#include "SafeEncButton.h"

#include "Config.h"
#include "DTEncoder.h"
#include "Display.h"
#include "Lightmeter.h"
#include "Settings.h"

extern ButtonT<SHOW_REL_BTN_PIN> gShowRelBtn;
extern ButtonT<ENCODER_BTN_PIN> gEncoderBtn;
extern VirtButton gSleepBtn;

extern Display gDisplay;
extern Lightmeter gLightmeter;
extern DTEncoder gEncoder;

extern Settings gSettings;

// owner of all Hardware devices
class Hardware {
public:
    void init();
    void tick();

    bool goingToSleep() { return m_goingToSleep; }

private:
    void sleep();
    static void wakeUp();
    void poweron();
    void poweroff();

    void startCalibration();

    bool m_disabled = false;
    bool m_goingToSleep = false;
    uint32_t m_lastActionTime;
    bool m_justWakedUp = false;
    uint32_t m_wakeUpTime;
};

extern Hardware gHardware;
