#include "Utils.h"

#include <assert.h>
#include <math.h>
#include <string.h>

float fastLog10(float x) {
    int exponent;
    float value = frexpf(fabsf(x), &exponent);
    float y = 1.23149591368684f;
    y *= value;
    y += -4.11852516267426f;
    y *= value;
    y += 6.02197014179219f;
    y *= value;
    y += -3.13396450166353f;
    y += exponent;
    return y * 0.3010299956639812f;
}

float fast2pow(float exp) {
    assert(exp >= 0);

    int8_t intPart = static_cast<int8_t>(exp);
    float fracPart = exp - intPart;

    // First 5 terms of Taylor series 2**(log2(e)*x)
    float mantisa =
        1.0f + fracPart * (0.69314718f +
                           fracPart * (0.24022651f +
                                       fracPart * (0.05550411f + fracPart * (0.00961813f + fracPart * 0.00133f))));

    return ldexpf(mantisa, intPart);
}
