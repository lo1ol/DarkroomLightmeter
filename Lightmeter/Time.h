#pragma once

#include <math.h>
#include <stdint.h>

class Time {
public:
    constexpr explicit Time() : m_sec(0) {}
    constexpr explicit Time(uint32_t sec) : m_sec(sec > 5999 ? 5999 : sec) {}

    [[nodiscard]] constexpr Time operator*(double x) const { return Time{ static_cast<uint32_t>(lround(m_sec * x)) }; }

    [[nodiscard]] constexpr bool operator>(const Time& o) const { return m_sec > o.m_sec; }
    [[nodiscard]] constexpr bool operator<(const Time& o) const { return m_sec < o.m_sec; }

    [[nodiscard]] constexpr uint16_t secs() const { return m_sec % 60; }
    [[nodiscard]] constexpr uint16_t mins() const { return m_sec / 60; }

    // private:
    uint16_t m_sec;
};

constexpr Time kMaxTime{ 5999 };
