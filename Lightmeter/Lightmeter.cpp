#include "Lightmeter.h"

#include <Arduino.h>
#include <Wire.h>

#include <ADS1X15.h>
#include <PinChangeInterrupt.h>

#include <assert.h>
#include <math.h>

#include "Hardware.h"
#include "Utils.h"

#define MAX_BRIGHTNESS float(int32_t(AMP_R_HIGH) * GAIN_AMP * (1 << 15))

namespace {
constexpr uint16_t gAmpMap[]{ AMP_R_HIGH * GAIN_AMP, AMP_R_HIGH, GAIN_AMP, 1 };

static uint16_t gAmpDarkVoltageMap[4]{};

uint16_t toLogD(const int32_t (&measures)[MEASURE_SOFT_CNT]) {
    int64_t res = 0;
    for (auto measure : measures)
        res += measure;

    if (res <= 16 * MEASURE_SOFT_CNT)
        return 999;

    return lround(fastLog10((MAX_BRIGHTNESS * MEASURE_SOFT_CNT) / res) * 100);
}

void buildDarkVoltageMap() {
    gAmpDarkVoltageMap[0] = gSettings.lowResistorDarkVoltageValue / GAIN_AMP;
    gAmpDarkVoltageMap[1] = gSettings.lowResistorDarkVoltageValue;
    gAmpDarkVoltageMap[2] = gSettings.highResistorDarkVoltageValue / GAIN_AMP;
    gAmpDarkVoltageMap[3] = gSettings.highResistorDarkVoltageValue;
}

} // namespace

Lightmeter::Lightmeter() : m_ads(0x48) {
    pinMode(ADC_MULTIMPLEXER_POWER_PIN, OUTPUT);
    pinMode(DIOD_POWER_PIN, OUTPUT);
    pinMode(MULTIPLEXER_LOW_RESISTOR_PIN, OUTPUT);
    buildDarkVoltageMap();
}

void Lightmeter::poweron() {
    digitalWrite(DIOD_POWER_PIN, HIGH);
    digitalWrite(ADC_MULTIMPLEXER_POWER_PIN, HIGH);

    Wire.begin();
    m_ads.begin();
    m_ads.setDataRate(ADS1X15_DATARATE_0);
    m_ads.setComparatorThresholdHigh(0x8000);
    m_ads.setComparatorThresholdLow(0x0000);
    m_ads.setComparatorQueConvert(0);
    m_ads.setMode(0);

    pinMode(ADC_READY_PIN, INPUT_PULLUP);
    attachPinChangeInterrupt(
        digitalPinToPinChangeInterrupt(ADC_READY_PIN), [] { gLightmeter.m_readyFlag = true; }, RISING);

    m_readyFlag = true;
    requestNextMeasure(AmpLevel::R_LOW_G0, true);
    delay(10);
}

void Lightmeter::poweroff() {
    digitalWrite(MULTIPLEXER_LOW_RESISTOR_PIN, LOW);
    detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ADC_READY_PIN));
    pinMode(A4, INPUT);
    pinMode(A5, INPUT);
    pinMode(ADC_READY_PIN, INPUT);

    digitalWrite(DIOD_POWER_PIN, LOW);
    digitalWrite(ADC_MULTIMPLEXER_POWER_PIN, LOW);
}

void Lightmeter::tick() {
    if (!m_readyFlag)
        return;

    if (m_dropNextValue) {
        m_dropNextValue = false;
        requestNextMeasure(m_ampLevel);
        return;
    }

    int32_t val = m_ads.getValue();
    int32_t adjustVal = val;

    adjustVal -= gAmpDarkVoltageMap[m_ampLevel];
    adjustVal *= gAmpMap[m_ampLevel];
    m_measureIteration = (m_measureIteration + 1) % MEASURE_SOFT_CNT;
    m_measures[m_measureIteration] = adjustVal;

    auto adjust = ampAdjust(val);
    requestNextMeasure(static_cast<AmpLevel>(static_cast<int8_t>(m_ampLevel) + adjust));
}

int8_t Lightmeter::ampAdjust(int16_t val) const {
    switch (m_ampLevel) {
    case AmpLevel::R_LOW_G0:
        if (val < 1000)
            return 1;
        return 0;
    case AmpLevel::R_LOW_G1:
        if (val > 31000)
            return -1;
        if (val < static_cast<int16_t>(1000 + gSettings.lowResistorDarkVoltageValue))
            return 1;
        return 0;
    case AmpLevel::R_HIGH_G0:
        if (val < 1000)
            return 1;
        if (val > 15000)
            return -1;
        return 0;
    case AmpLevel::R_HIGH_G1:
        if (val > 31000)
            return -1;
        return 0;
    }

    return 0;
}

void Lightmeter::requestNextMeasure(AmpLevel amp, bool forceSetAmp) {
    while (!m_readyFlag) {}

    if (m_ampLevel != amp || forceSetAmp) {
        bool needHighResistor = amp > AmpLevel::R_LOW_G1;
        digitalWrite(MULTIPLEXER_LOW_RESISTOR_PIN,
                     needHighResistor ? !MULTIPLEXER_LOW_RESISTOR_VALUE : MULTIPLEXER_LOW_RESISTOR_VALUE);

        m_dropNextValue = true;
    }

    m_ampLevel = amp;

    bool needGain = amp == AmpLevel::R_LOW_G1 || amp == AmpLevel::R_HIGH_G1;
    m_ads.setGain(needGain ? 16 : 0);

    m_readyFlag = false;
    m_ads.requestADC_Differential_0_3();
}

uint16_t Lightmeter::getLastMeasure() const {
    return toLogD(m_measures);
}

int16_t Lightmeter::syncGetVal(AmpLevel amp) {
again:
    bool needDrop = amp != m_ampLevel || m_dropNextValue;

    while (!m_readyFlag) {}
    auto val = m_ads.getValue();
    m_dropNextValue = false;

    requestNextMeasure(amp);

    if (needDrop)
        goto again;

    return val;
}

bool Lightmeter::calibrate() {
    auto newSettings = gSettings;

    constexpr uint8_t kTestsCnt = 100;

    for (int8_t lowResistor = 0; lowResistor != 2; ++lowResistor) {
        delay(5000);
        auto amp = lowResistor ? AmpLevel::R_LOW_G1 : AmpLevel::R_HIGH_G1;

        int32_t res = 0;
        for (uint8_t i = 0; i != kTestsCnt; ++i) {
            auto val = syncGetVal(amp);

            if (val > 5000)
                return false;

            res += val;
        }

        res /= double(kTestsCnt);

        if (lowResistor)
            newSettings.lowResistorDarkVoltageValue = res;
        else
            newSettings.highResistorDarkVoltageValue = res;
    }

    gSettings = newSettings;
    gSettings.updateEEPROM();
    buildDarkVoltageMap();

    return true;
}
