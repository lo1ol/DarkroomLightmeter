#include "SafeEncButton.h"

#include <gio/gio.h>

#include <assert.h>
#include <math.h>

#include "Hardware.h"
#include "Lightmeter.h"
#include "Utils.h"

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
        if (gEncoder.getInt(gSettings.baseLogD, 20, 599)) {
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
    static uint16_t gLastShownAbsVal;

    gHardware.tick();

    if (gHardware.goingToSleep()) {
        if (gMode == Mode::SetBase)
            finishSetBase();
        gMode = Mode::ShowAbs;
        return;
    }

    // used only for ShowAbs and ShowRel
    bool needUpdateDisplay = false;

    switch (gMode) {
    case Mode::ShowAbs:
        if (gShowRelBtn.click()) {
            needUpdateDisplay = true;
            gMode = Mode::ShowRel;
        } else if (gEncoderBtn.click()) {
            gMode = Mode::ShowTime;
            gDisplay.showTime(calcSuggestedTime(gLastShownAbsVal));
        } else if (gEncoderBtn.hold() && !gSleepBtn.pressing()) {
            startSetBase();
            gMode = Mode::SetBase;
        }
        break;
    case Mode::ShowRel:
        if ((gShowRelBtn.hold() || gEncoderBtn.hold()) && !gSleepBtn.pressing()) {
            gMode = Mode::ShowAbs;
            needUpdateDisplay = true;
        }
        break;
    case Mode::ShowTime:
        if (gEncoderBtn.click() || (gEncoderBtn.hold() && !gSleepBtn.pressing())) {
            needUpdateDisplay = true;
            gMode = Mode::ShowAbs;
        }
        return;
    case Mode::SetBase:
        if (gEncoderBtn.hold() && !gSleepBtn.pressing()) {
            gMode = Mode::ShowAbs;
        }

        handleSetBase();
        return;
    }

    auto measure = gLightmeter.getLastMeasure();
    static auto gLastMeasure = measure;

    // This block is used to control stability of value changing
    // If values increasing or decreasing stadily
    // than we can update values on display faster
    //
    // But for not stable changes we need to wait more time
    if (gLightmeter.hasUpdates()) {
        static bool gValuesIsIncreasing = false;
        static uint8_t gStabilityScore = 0;

        auto newDir = gValuesIsIncreasing;

        if (measure > gLastMeasure)
            newDir = true;
        if (measure < gLastMeasure)
            newDir = false;

        if (gValuesIsIncreasing != newDir)
            gStabilityScore = 0;

        gValuesIsIncreasing = newDir;

        if (gStabilityScore == 3) {
            needUpdateDisplay |= millis() - gLastShowTime > 50;
        } else {
            ++gStabilityScore;
            needUpdateDisplay |= millis() - gLastShowTime > 250;
        }
    }

    if (!needUpdateDisplay)
        return;

    gLastShowTime = millis();
    gLastMeasure = measure;

    switch (gMode) {
    case Mode::ShowAbs:
        gLastShownAbsVal = measure;
        gDisplay.showVal(measure);
        break;
    case Mode::ShowRel:
        gDisplay.showRelVal(measure - gLastShownAbsVal);
        break;
    case Mode::ShowTime:
    case Mode::SetBase:
        assert(false);
        break;
    }
}
