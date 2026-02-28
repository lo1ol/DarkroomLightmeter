#include "Hardware.h"

ButtonT<SHOW_REL_BTN_PIN> gShowRelBtn;
ButtonT<SHOW_TIME_BTN_PIN> gShowTimeBtn;
ButtonT<ENCODER_BTN_PIN> gEncoderBtn;

Settings gSettings = Settings::load();
Display gDisplay;
Lightmeter gLightmeter;
DTEncoder gEncoder;
