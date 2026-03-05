#include "Lightmeter.h"

#include <Arduino.h>
#include <Wire.h>

#include <ADS1X15.h>
#include <PinChangeInterrupt.h>

#include <assert.h>
#include <math.h>

#include "Hardware.h"
#include "Utils/Utils.h"

#define MAX_BRIGHTNESS float(int32_t(AMP_R1) * GAIN_AMP * (1 << 15))

namespace {
constexpr uint16_t gAmpMap[]{ AMP_R1 * GAIN_AMP, AMP_R1, GAIN_AMP, 1 };

constexpr uint16_t gAmpDarkVoltageMap[]{
    DARK_VOLTAGE_R0 / GAIN_AMP,
    DARK_VOLTAGE_R0,
    DARK_VOLTAGE_R1 / GAIN_AMP,
    DARK_VOLTAGE_R1,
};

uint16_t toLogD(const int32_t (&kMeasures)[MEASURE_SOFT_CNT]) {
    int64_t res = 0;
    for (auto measure : kMeasures)
        res += measure;

    if (res <= 0)
        return 999;

    return lround(fastLog10((MAX_BRIGHTNESS * MEASURE_SOFT_CNT) / res) * 100);
}
} // namespace

Lightmeter::Lightmeter() : m_ads(0x48) {
    pinMode(ADC_POWER_PIN, OUTPUT);
    pinMode(DIOD_POWER_PIN, OUTPUT);
    pinMode(MULTIPLEXER_POWER_PIN, OUTPUT);

    pinMode(MULTIPLEXER_R1_PIN, OUTPUT);
    digitalWrite(MULTIPLEXER_R1_PIN, HIGH); // TODO it's enable R0
}

void Lightmeter::poweron() {
    m_ampLevel = AmpLevel::R0_G0;
    digitalWrite(DIOD_POWER_PIN, HIGH);
    digitalWrite(MULTIPLEXER_R1_PIN, HIGH); // TODO it's enable R0
    digitalWrite(ADC_POWER_PIN, HIGH);
    digitalWrite(MULTIPLEXER_POWER_PIN, HIGH);
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
    m_readDiodVoltage = true;
    requestNextMeasure();
}

void Lightmeter::poweroff() {
    pinMode(A4, INPUT);
    pinMode(A5, INPUT);
    m_ampLevel = AmpLevel::R0_G0;
    digitalWrite(DIOD_POWER_PIN, LOW);
    digitalWrite(ADC_POWER_PIN, LOW);
    digitalWrite(MULTIPLEXER_R1_PIN, LOW);
    digitalWrite(MULTIPLEXER_POWER_PIN, LOW);
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

    if (m_readDiodVoltage) {
        m_readDiodVoltage = false;
        m_maxAdsVal = val - (1.3 / 6.144) * (1 << 15); // remove voltage drop on diod
    } else {
        int32_t adjustVal = val;

        adjustVal -= gAmpDarkVoltageMap[m_ampLevel];
        adjustVal *= gAmpMap[m_ampLevel];
        m_measureIteration = (m_measureIteration + 1) % MEASURE_SOFT_CNT;
        m_measures[m_measureIteration] = adjustVal;

        auto adjust = ampAdjust(val);
        if (adjust) {
            reinterpret_cast<int8_t&>(m_ampLevel) += adjust;

            bool highResistor = m_ampLevel > AmpLevel::R0_G1;
            digitalWrite(MULTIPLEXER_R1_PIN, !highResistor);
            m_dropNextValue = true;
        }
    }

    requestNextMeasure();
}

int8_t Lightmeter::ampAdjust(uint16_t val) const {
    switch (m_ampLevel) {
    case AmpLevel::R0_G0:
        if (val < 1000)
            return 1;
        return 0;
    case AmpLevel::R0_G1:
        if (val > 31000)
            return -1;
        if (val < 1800)
            return 1;
        return 0;
    case AmpLevel::R1_G0:
        if (val < 1000)
            return 1;
        if (val > m_maxAdsVal * 0.9)
            return -1;
        return 0;
    case AmpLevel::R1_G1:
        if (val > 31000)
            return -1;
        return 0;
    }

    return 0;
}

void Lightmeter::requestNextMeasure() {
    if (m_readDiodVoltage) {
        m_ads.setGain(0);
        m_ads.requestADC_Differential_1_3();
    } else {
        bool needGain = m_ampLevel == AmpLevel::R0_G1 || m_ampLevel == AmpLevel::R1_G1;
        m_ads.setGain(needGain ? 16 : 0);
        m_ads.requestADC_Differential_0_3();
    }

    m_readyFlag = false;
}

uint16_t Lightmeter::getLastMeasure() const {
    return toLogD(m_measures);
}
