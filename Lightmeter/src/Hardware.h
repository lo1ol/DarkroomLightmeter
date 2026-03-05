#pragma once

#include "Utils/SafeEncButton.h"

#include "Config.h"
#include "DTEncoder.h"
#include "Display.h"
#include "Lightmeter.h"
#include "Settings.h"

extern ButtonT<SHOW_REL_BTN_PIN> gShowRelBtn;
extern ButtonT<ENCODER_BTN_PIN> gEncoderBtn;

extern Display gDisplay;
extern Lightmeter gLightmeter;
extern DTEncoder gEncoder;

extern Settings gSettings;

// owner of all Hardware devices
class Hardware {
public:
    void init();
    void tick();

private:
    void sleep();
    static void wakeUp();

    bool m_disabled = false;
    uint32_t m_lastActionTime;
};

extern Hardware gHardware;
