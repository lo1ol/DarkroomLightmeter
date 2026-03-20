#include "Lightmeter.h"

#include <Arduino.h>
#include <Wire.h>

#include <ADS1X15.h>
#include <PinChangeInterrupt.h>

#include <assert.h>
#include <math.h>

#include "Hardware.h"
#include "Utils/Utils.h"

#define MAX_BRIGHTNESS float(int32_t(AMP_R_HIGH) * GAIN_AMP * (1 << 15))

namespace {
constexpr uint16_t gAmpMap[]{ AMP_R_HIGH * GAIN_AMP, AMP_R_HIGH, GAIN_AMP, 1 };

static uint16_t gAmpDarkVoltageMap[4]{};

uint16_t toLogD(const int32_t (&kMeasures)[MEASURE_SOFT_CNT]) {
    int64_t res = 0;
    for (auto measure : kMeasures)
        res += measure;

    if (res <= 0)
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
    pinMode(MULTIPLEXER_LOW_RESISTOR_PIN, OUTPUT);
    digitalWrite(MULTIPLEXER_LOW_RESISTOR_PIN, MULTIPLEXER_LOW_RESISTOR_VALUE);
    buildDarkVoltageMap();
}

void Lightmeter::poweron() {
    m_ampLevel = AmpLevel::R_LOW_G0;
    digitalWrite(MULTIPLEXER_LOW_RESISTOR_PIN, MULTIPLEXER_LOW_RESISTOR_VALUE);
    pinMode(ADC_READY_PIN, INPUT_PULLUP);
    delay(10);

    Wire.begin();
    m_ads.begin();
    m_ads.setDataRate(ADS1X15_DATARATE_0);
    m_ads.setComparatorThresholdHigh(0x8000);
    m_ads.setComparatorThresholdLow(0x0000);
    m_ads.setComparatorQueConvert(0);

    attachPinChangeInterrupt(
        digitalPinToPinChangeInterrupt(ADC_READY_PIN), [] { gLightmeter.m_readyFlag = true; }, RISING);

    m_ads.setMode(0);
    requestNextMeasure();
    delay(10);
}

void Lightmeter::poweroff() {
    pinMode(A4, INPUT);
    pinMode(A5, INPUT);
    m_ampLevel = AmpLevel::R_LOW_G0;
    digitalWrite(MULTIPLEXER_LOW_RESISTOR_PIN, LOW);
    pinMode(ADC_READY_PIN, INPUT);
}

void Lightmeter::tick() {
    if (!m_readyFlag)
        return;

    if (m_dropNextValue) {
        m_dropNextValue = false;
        requestNextMeasure();
        return;
    }

    int32_t val = m_ads.getValue();
    int32_t adjustVal = val;

    adjustVal -= gAmpDarkVoltageMap[m_ampLevel];
    adjustVal *= gAmpMap[m_ampLevel];
    m_measureIteration = (m_measureIteration + 1) % MEASURE_SOFT_CNT;
    m_measures[m_measureIteration] = adjustVal;

    auto adjust = ampAdjust(val);
    if (adjust) {
        reinterpret_cast<int8_t&>(m_ampLevel) += adjust;

        bool highResistor = m_ampLevel > AmpLevel::R_LOW_G1;
        digitalWrite(MULTIPLEXER_LOW_RESISTOR_PIN,
                     highResistor ? !MULTIPLEXER_LOW_RESISTOR_VALUE : MULTIPLEXER_LOW_RESISTOR_VALUE);
        m_dropNextValue = true;
    }

    requestNextMeasure();
}

int8_t Lightmeter::ampAdjust(uint16_t val) const {
    switch (m_ampLevel) {
    case AmpLevel::R_LOW_G0:
        if (val < 1000)
            return 1;
        return 0;
    case AmpLevel::R_LOW_G1:
        if (val > 31000)
            return -1;
        if (val < (1000 + gSettings.lowResistorDarkVoltageValue))
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

void Lightmeter::requestNextMeasure() {
    m_readyFlag = false;

    bool needGain = m_ampLevel == AmpLevel::R_LOW_G1 || m_ampLevel == AmpLevel::R_HIGH_G1;
    m_ads.setGain(needGain ? 16 : 0);
    m_ads.requestADC_Differential_0_3();
}

uint16_t Lightmeter::getLastMeasure() const {
    return toLogD(m_measures);
}

bool Lightmeter::calibrate() {
    constexpr uint8_t kTestsCnt = 100;

    for (int8_t highResistor = 0; highResistor != 2; ++highResistor) {
        delay(5000);
        if (highResistor) {
            m_ampLevel = AmpLevel::R_HIGH_G1;
            digitalWrite(MULTIPLEXER_LOW_RESISTOR_PIN, !MULTIPLEXER_LOW_RESISTOR_VALUE);
        } else {
            m_ampLevel = AmpLevel::R_LOW_G1;
            digitalWrite(MULTIPLEXER_LOW_RESISTOR_PIN, MULTIPLEXER_LOW_RESISTOR_VALUE);
        }

        uint32_t res = 0;
        uint8_t i = 0;

        // drop first value goten on wront resistor
        // drop second value after changing a resistor
        while (i != (kTestsCnt + 2)) {
            if (!m_readyFlag)
                continue;

            auto val = m_ads.getValue();
            if (i > 1) {
                if (val > 5000)
                    return false;

                res += m_ads.getValue(); // drop first value
            }

            ++i;
            requestNextMeasure();
        }

        res /= kTestsCnt;

        if (highResistor)
            gSettings.highResistorDarkVoltageValue = res;
        else
            gSettings.lowResistorDarkVoltageValue = res;
    }

    gSettings.updateEEPROM();
    buildDarkVoltageMap();

    return true;
}
