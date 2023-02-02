#include "variables.h"

byte currentAutoPulseType = 255;
byte lastAutoPulseNode = 255;
unsigned long lastAutoPulseChange = millis();

float decay = 0.97; // Multiply all LED's by this amount each tick to create fancy fading tails

const uint16_t g_hue = 40000;
unsigned int localPort = 8888; // local port to listen on
unsigned int debugPort = 8000; // local port to listen on
bool connected = false;

bool activeOTAUpdate = false;