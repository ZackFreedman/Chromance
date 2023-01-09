/*
   A dot animation that travels along rails
   (C) Voidstar Lab LLC 2021
*/

#ifndef RIPPLE_H_
#define RIPPLE_H_

// WARNING: These slow things down enough to affect performance. Don't turn on unless you need them!
// #define DEBUG_ADVANCEMENT // Print debug messages about ripples' movement
// #define DEBUG_RENDERING  // Print debug messages about translating logical to actual position

#include "variables.h"

#ifdef USING_DOTSTAR
#include <Adafruit_DotStar.h>
#endif
#ifdef USING_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#endif

#include "mapping.h"

enum rippleState
{
  STATE_DEAD,        // Ripple is to be deleted and should not lit up
  STATE_WITHIN_NODE, // Ripple isn't drawn as it passes through a node to keep the speed consistent
  STATE_TRAVEL_UP,   // Ripple is moving upward
  STATE_TRAVEL_DOWN  // Ripple is moving downward
};

enum rippleBehavior
{
  BEHAVIOR_COUCH_POTATO, // Stop at next node
  BEHAVIOR_LAZY,         // Only go straight
  BEHAVIOR_WEAK,         // Go straight if possible
  BEHAVIOR_FEISTY,
  BEHAVIOR_ANGRY,
  BEHAVIOR_ALWAYS_RIGHT,
  BEHAVIOR_ALWAYS_LEFT,
  BEHAVIOR_EXPLODING // Coming soon
};

float fmap(float x, float in_min, float in_max, float out_min, float out_max);

class Ripple
{

public:
  Ripple(int id);
  void start(int node, int direction, unsigned long color, float speed, unsigned long lifespan, rippleBehavior behavior);
  void advance(byte ledColors[NUMBER_OF_SEGMENTS][LEDS_PER_SEGMENTS][3]);

  rippleState state = STATE_DEAD;
  unsigned long color;
  /*
  If within a node: 0 is node, 1 is direction
  If traveling, 0 is segment, 1 is LED position from bottom
  */
  int node;
  int direction;

private:
  void renderLed(byte ledColors[NUMBER_OF_SEGMENTS][LEDS_PER_SEGMENTS][3], unsigned long age);

  float speed;            // Each loop, ripples move this many LED's.
  unsigned long lifespan; // The ripple stops after this many milliseconds
  /*
  0: Always goes straight ahead if possible
  1: Can take 60-degree turns
  2: Can take 120-degree turns
      */
  rippleBehavior behavior;
  bool justStarted = false;
  float pressure;          // When Pressure reaches 1, ripple will move
  unsigned long birthday;  // Used to track age of ripple
  static byte rippleCount; // Used to give them unique ID's
  byte rippleId;           // Used to identify this ripple in debug output
};

#endif
