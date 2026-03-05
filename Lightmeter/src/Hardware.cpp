#include "Hardware.h"

#include <GyverPower.h>
#include <PinChangeInterrupt.h>

ButtonT<SHOW_REL_BTN_PIN> gShowRelBtn;
ButtonT<ENCODER_BTN_PIN> gEncoderBtn;

Settings gSettings = Settings::load();
Display gDisplay;
Lightmeter gLightmeter;
DTEncoder gEncoder;
Hardware gHardware;

void Hardware::init() {
    power.setSleepMode(POWERDOWN_SLEEP);
    power.autoCalibrate();
    power.setSleepResolution(SLEEP_8192MS);

    gEncoder.poweron();
    gDisplay.poweron();
    gLightmeter.poweron();
    gSettings = Settings::load();

    m_lastActionTime = millis();
}

void Hardware::tick() {
    gDisplay.tick();
    gLightmeter.tick();

    if (gEncoder.tick() || gShowRelBtn.tick() || gEncoderBtn.tick())
        m_lastActionTime = millis();

    if (static_cast<uint32_t>(millis() - m_lastActionTime) > AUTOSLEEP_TIME)
        sleep();
}

void Hardware::wakeUp() {
    if (!gHardware.m_disabled)
        return;

    gHardware.m_disabled = false;

    if (power.inSleep())
        power.wakeUp();
}

void Hardware::sleep() {
    gDisplay.poweroff();
    gLightmeter.poweroff();
    gEncoder.poweroff();

    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ENCODER_CLK_PIN), Hardware::wakeUp, CHANGE);
    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ENCODER_BTN_PIN), Hardware::wakeUp, FALLING);
    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SHOW_REL_BTN_PIN), Hardware::wakeUp, FALLING);

    m_disabled = true;

    power.hardwareDisable(PWR_ALL);
    while (m_disabled)
        power.sleepDelay(60 * 60 * 1000L);

    power.hardwareEnable(PWR_ALL);
    power.hardwareDisable(PWR_SPI | PWR_USB | PWR_ADC | PWR_TIMER1 | PWR_TIMER2 | PWR_UART1 | PWR_UART2 | PWR_UART3);

    detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ENCODER_DT_PIN));
    detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ENCODER_BTN_PIN));
    detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SHOW_REL_BTN_PIN));

    gDisplay.poweron();
    gEncoder.poweron();
    gLightmeter.poweron();

    m_lastActionTime = millis();
}
