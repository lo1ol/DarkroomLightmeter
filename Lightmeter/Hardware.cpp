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

    pinMode(ADC_MULTIMPLEXER_POWER_PIN, OUTPUT);
    pinMode(DISPLAY_POWER_PIN, OUTPUT);
    pinMode(DIOD_POWER_PIN, OUTPUT);

    poweron();

    gSettings = Settings::load();
}

void Hardware::tick() {
again:
    gDisplay.tick();
    gLightmeter.tick();

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

    if (m_goToSleep || static_cast<uint32_t>(millis() - m_lastActionTime) > AUTOSLEEP_TIME) {
        m_goToSleep = false;
        sleep();
        goto again;
    }
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
    digitalWrite(DISPLAY_POWER_PIN, LOW);
    digitalWrite(DIOD_POWER_PIN, LOW);
    digitalWrite(ADC_MULTIMPLEXER_POWER_PIN, LOW);

    gDisplay.poweroff();
    gLightmeter.poweroff();
    gEncoder.poweroff();

    uint32_t holdTime = millis();

    // wait till user stop doing actions
    while (gShowRelBtn.tick() || gEncoderBtn.tick() || gShowRelBtn.pressing() || gEncoderBtn.pressing()) {
        if (millis() - holdTime < 5000)
            continue;
        startCalibration();
        return;
    }

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

    digitalWrite(DISPLAY_POWER_PIN, HIGH);
    digitalWrite(ADC_MULTIMPLEXER_POWER_PIN, HIGH);
    digitalWrite(DIOD_POWER_PIN, HIGH);

    gEncoder.poweron();
    gDisplay.poweron();
    gLightmeter.poweron();

    m_justWakedUp = true;
    m_wakeUpTime = millis();
    m_lastActionTime = millis();
}

void Hardware::startCalibration() {
    poweron();
    gDisplay.showCalibrationAnim();
    bool res = gLightmeter.calibrate();
    gDisplay.showRes(res);
}
