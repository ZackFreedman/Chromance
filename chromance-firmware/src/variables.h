/*
 * Maps hex topology onto LED's
 * (C) Voidstar Lab LLC 2021
 */

#ifndef VARIABLES_H_
#define VARIABLES_H_

#include <WiFi.h>

// TODO: USER SET
// COMMENT OUT IF NOT USING
// #define USING_EMOTI_BIT_SENSOR

#define HOSTNAME "Chromance" ///< Hostname. The setup function adds the Chip ID at the end.
#define NUMBER_OF_RIPPLES 30

#define NUMBER_OF_NODES 25
#define MAX_SIDES_PER_NODES 6

#define NUMBER_OF_SEGMENTS 40
#define SIDES_PER_SEGMENT 2
#define NUM_OF_PIXELS (NUMBER_OF_SEGMENTS * LEDS_PER_SEGMENTS)

#define LEDS_PER_SEGMENTS 14

#define NUMBER_OF_STRIPS 4
#define NUMBER_OF_ANIMATIONS 3

#define BLUE_LENGTH 154
#define GREEN_LENGTH 168
#define RED_LENGTH 84
#define BLACK_LENGTH 154

#define BLUE_INDEX 0
#define GREEN_INDEX 1
#define RED_INDEX 2
#define BLACK_INDEX 3

// CONVERT
// BLUE = RED
// GREEN = YELLOW
// RED = GREEN
// BLACK = BLUE

// TODO: USER SET
// Data Pins FILL IN WITH YOUR PIN NUMBERS
#define BLUE_STRIP_DATA_PIN 33
#define GREEN_STRIP_DATA_PIN 32
#define RED_STRIP_DATA_PIN 2
#define BLACK_STRIP_DATA_PIN 4

// TODO: USER SET
// [DOTSTAR ONLY] Clock Pins  FILL IN WITH YOUR PIN NUMBERS IF USING DOTSTAR
#ifdef USING_DOTSTAR
#define BLUE_STRIP_CLOCK_PIN 2
#define GREEN_STRIP_CLOCK_PIN 4
#define RED_STRIP_CLOCK_PIN 17
#define BLACK_STRIP_CLOCK_PIN 18
#endif

// If you don't have an EmotiBit or don't feel like wearing it, that's OK
// We'll fire automatic pulses
#define randomPulsesEnabled true    // Fire random rainbow pulses from random nodes
#define cubePulsesEnabled true      // Draw cubes at random nodes
#define starburstPulsesEnabled true // Draw starbursts

#define randomPulseTime 2000  // Fire a random pulse every (this many) ms

extern const uint16_t g_hue;
extern unsigned int localPort; // local port to listen on
extern unsigned int debugPort; // local port to listen on
extern bool connected;

extern float decay; // Multiply all LED's by this amount each tick to create fancy fading tails

extern byte lastAutoPulseNode;

extern byte currentAutoPulseType;
extern unsigned long lastAutoPulseChange;
extern bool activeOTAUpdate;

#define DOTSTAR 1
#define NEOPIXEL 2
// TODO: USER SET
// SET TO DOTSTAR OR NEOPIXEL, DEPENDING ON WHAT YOU ARE USING
#define __LED_TYPE NEOPIXEL

#if __LED_TYPE == DOTSTAR
#define USING_DOTSTAR
#undef USING_NEOPIXEL
#elif __LED_TYPE == NEOPIXEL
#define USING_NEOPIXEL
#undef USING_DOTSTAR
#endif

#endif