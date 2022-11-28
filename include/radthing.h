/*
    Contains hardware specific information for the base microcontroller board used. 
*/
#ifndef ESP8266THING_H
#define ESP8266THING_H

#include "RadiationWatch.h"

#define DISABLE_SERIAL_OUTPUT 1

// The onboard LED controls for the ESP8266 seem counter-intuitive (high for off), so map them to explicit labels
#define LED_OFF HIGH
#define LED_ON LOW

// RadiationWatch
#define SIG_PIN 12 
#define NS_PIN 4

#define DEVICE_ID "esp8266thing"
#define DEVICE_NAME "RadiationWatcher"
#define DEVICE_MANUFACTURER "Sparkfun"
#define DEVICE_MODEL "ESP8266 Thing Dev"
#define DEVICE_VERSION "20221127.1800"

// *********************************************************************************************************************
// *** Must Declare ***
extern RadiationWatch radiationWatch;

#endif