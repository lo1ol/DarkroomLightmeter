#include "Hardware.h"

#include <GyverPower.h>
#include <PinChangeInterrupt.h>

ButtonT<SHOW_REL_BTN_PIN> gShowRelBtn;
ButtonT<ENCODER_BTN_PIN> gEncoderBtn;
VirtButton gSleepBtn;

Settings gSettings = Settings::load();
Display gDisplay;
Lightmeter gLightmeter;
DTEncoder gEncoder;
Hardware gHardware;

void Hardware::init() {
    power.setSleepMode(POWERDOWN_SLEEP);
    power.autoCalibrate();
    power.setSleepResolution(SLEEP_8192MS);

    poweron();

    gSettings = Settings::load();
}

void Hardware::tick() {
    gDisplay.tick();
    gLightmeter.tick();

    if (m_goingToSleep) {
        sleep();
        m_goingToSleep = false;
    }

    if (m_justWakedUp)
        m_justWakedUp = static_cast<uint32_t>(millis()) - m_wakeUpTime < 100;

    if (gShowRelBtn.tick()) {
        m_lastActionTime = millis();
        if (m_justWakedUp)
            gShowRelBtn.skipEvents();
    }

    if (gEncoderBtn.tick()) {
        m_lastActionTime = millis();
        if (m_justWakedUp)
            gEncoderBtn.skipEvents();
    }

    if (gEncoder.tick())
        m_lastActionTime = millis();

    gSleepBtn.tick(gEncoderBtn, gShowRelBtn);

    if (gSleepBtn.hold() || static_cast<uint32_t>(millis() - m_lastActionTime) > AUTOSLEEP_TIME)
        m_goingToSleep = true;
}

void Hardware::wakeUp() {
    if (!gHardware.m_disabled)
        return;

    gHardware.m_disabled = false;

    if (power.inSleep())
        power.wakeUp();
}

void Hardware::sleep() {
    poweroff();

    while (m_disabled)
        power.sleepDelay(60 * 60 * 1000L);

    poweron();
}

void Hardware::poweroff() {
    gDisplay.poweroff();
    gLightmeter.poweroff();
    gEncoder.poweroff();

    uint32_t holdTime = millis();

    // wait till user stop doing actions
    while (gShowRelBtn.tick() || gEncoderBtn.tick() || gShowRelBtn.pressing() || gEncoderBtn.pressing()) {
        if (millis() - holdTime < 5000)
            continue;

        gDisplay.poweron();
        gLightmeter.poweron();
        gEncoder.poweron();
        startCalibration();
        return;
    }

    delay(100);

    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ENCODER_CLK_PIN), Hardware::wakeUp, CHANGE);
    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ENCODER_BTN_PIN), Hardware::wakeUp, FALLING);
    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SHOW_REL_BTN_PIN), Hardware::wakeUp, FALLING);

    m_disabled = true;

    power.hardwareDisable(PWR_ALL);
}

void Hardware::poweron() {
    power.hardwareEnable(PWR_ALL);
    power.hardwareDisable(PWR_SPI | PWR_USB | PWR_ADC | PWR_TIMER1 | PWR_TIMER2 | PWR_UART1 | PWR_UART2 | PWR_UART3);

    detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ENCODER_DT_PIN));
    detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ENCODER_BTN_PIN));
    detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SHOW_REL_BTN_PIN));

    gDisplay.poweron();
    gEncoder.poweron();
    gLightmeter.poweron();

    m_justWakedUp = true;
    m_wakeUpTime = millis();
    m_lastActionTime = millis();
}

void Hardware::startCalibration() {
    gDisplay.showCalibrationAnim();
    bool res = gLightmeter.calibrate();
    gDisplay.showRes(res);
}
