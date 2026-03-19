#include "Utils/SafeEncButton.h"

#include <gio/gio.h>

#include <assert.h>
#include <math.h>

#include "Hardware.h"
#include "Lightmeter.h"
#include "Utils/Utils.h"

namespace {

enum class SetBaseStage { LodD, Min, Sec, last_ };

SetBaseStage gSetBaseStage = SetBaseStage::LodD;

void startSetBase() {
    gSetBaseStage = SetBaseStage::LodD;
    gDisplay.showVal(gSettings.baseLogD, Display::ShowValMode::Set);
}

void handleSetBase() {
    if (gEncoderBtn.click()) {
        gSetBaseStage = ADD_TO_ENUM(SetBaseStage, gSetBaseStage, 1);
        switch (gSetBaseStage) {
        case SetBaseStage::LodD:
            gDisplay.showVal(gSettings.baseLogD, Display::ShowValMode::Set);
            break;
        case SetBaseStage::Min:
            gDisplay.showTime(gSettings.baseTime, Display::ShowTimeMode::SetMin);
            break;
        case SetBaseStage::Sec:
            gDisplay.showTime(gSettings.baseTime, Display::ShowTimeMode::SetSec);
            gDisplay.resetBlink(true);
            break;
        case SetBaseStage::last_:
            break;
        };
    }

    switch (gSetBaseStage) {
    case SetBaseStage::LodD:
        if (gEncoder.getInt(gSettings.baseLogD, 20, 600)) {
            gDisplay.showVal(gSettings.baseLogD, Display::ShowValMode::Set);
        }
        break;
    case SetBaseStage::Min: {
        uint16_t mins = gSettings.baseTime.mins();
        if (!gEncoder.getInt(mins, 0, 59, true))
            break;

        gSettings.baseTime = Time(mins * 60 + gSettings.baseTime.secs());
        gDisplay.showTime(gSettings.baseTime, Display::ShowTimeMode::SetMin);
    } break;
    case SetBaseStage::Sec: {
        uint16_t secs = gSettings.baseTime.secs();
        if (!gEncoder.getInt(secs, 0, 59, true))
            break;

        gSettings.baseTime = Time(gSettings.baseTime.mins() * 60 + secs);
        gDisplay.showTime(gSettings.baseTime, Display::ShowTimeMode::SetSec);
    } break;
    case SetBaseStage::last_:
        break;
    };
}

void finishSetBase() {
    gSettings.updateEEPROM();
}

Time calcSuggestedTime(uint16_t logD) {
    return gSettings.baseTime * pow(2, static_cast<int16_t>(logD - gSettings.baseLogD) / 30.10299957f);
}

} // namespace

void setup() {
    gHardware.init();
}

enum class Mode {
    ShowAbs,
    ShowRel,
    ShowTime,
    SetBase,
};

void loop() {
    static Mode gMode = Mode::ShowAbs;
    static uint32_t gLastShowTime = 0;
    static uint16_t gRelMeasure;
    static uint16_t gLastShownAbsVal;

    gHardware.tick();

    bool needUpdateDisplay = true;
    if ((gMode == Mode::ShowAbs || gMode == Mode::ShowRel) && gShowRelBtn.click()) {
        gRelMeasure = gLightmeter.getLastMeasure();
        gMode = Mode::ShowRel;
    } else if (gMode == Mode::ShowAbs && gEncoderBtn.click()) {
        gMode = Mode::ShowTime;
    } else if (gMode == Mode::ShowTime && gEncoderBtn.click()) {
        gMode = Mode::ShowAbs;
    } else if (gMode == Mode::ShowAbs && gEncoderBtn.hold() && !gSleepBtn.pressing()) {
        startSetBase();
        gMode = Mode::SetBase;
    } else if (gMode == Mode::SetBase && gEncoderBtn.hold() && !gSleepBtn.pressing()) {
        finishSetBase();
        gMode = Mode::ShowAbs;
    } else if (gSleepBtn.hold()) {
        if (gMode == Mode::SetBase)
            finishSetBase();
        gMode = Mode::ShowAbs;
        gHardware.goToSleep();
    } else if ((gShowRelBtn.hold() || gEncoderBtn.hold()) && !gSleepBtn.pressing()) {
        gMode = Mode::ShowAbs;
    } else {
        needUpdateDisplay = false;
    }

    if (gMode == Mode::SetBase) {
        handleSetBase();
        return;
    }

    needUpdateDisplay |= millis() - gLastShowTime > 250;

    if (!needUpdateDisplay)
        return;

    gLastShowTime = millis();

    switch (gMode) {
    case Mode::ShowAbs:
        gLastShownAbsVal = gLightmeter.getLastMeasure();
        gDisplay.showVal(gLastShownAbsVal);
        break;
    case Mode::ShowRel:
        gDisplay.showRelVal(gLightmeter.getLastMeasure() - gRelMeasure);
        break;
    case Mode::ShowTime:
        gDisplay.showTime(calcSuggestedTime(gLastShownAbsVal));
        break;
    case Mode::SetBase:
        assert(false);
        break;
    }
}
