/*
   Chromance wall hexagon source (emotion controlled w/ EmotiBit)
   Partially cribbed from the DotStar example
   I smooshed in the ESP32 BasicOTA sketch, too

   (C) Voidstar Lab 2021
*/

//#include <Adafruit_DotStar.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "mapping.h"
#include "ripple.h"

#define HOSTNAME "ESP8266-OTA-" ///< Hostname. The setup function adds the Chip ID at the end.
#define NUMBER_OF_RIPPLES 30
// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const IPAddress ip(192, 168, 5, 50);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 5, 1);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)
const int port = 23;

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 2


/*
int lengths[] = {154, 168, 84, 154};  // Strips are different lengths because I am a dumb

Adafruit_DotStar strip0(lengths[0], 15, 2, DOTSTAR_BRG);
Adafruit_DotStar strip1(lengths[1], 0, 4, DOTSTAR_BRG);
Adafruit_DotStar strip2(lengths[2], 16, 17, DOTSTAR_BRG);
Adafruit_DotStar strip3(lengths[3], 5, 18, DOTSTAR_BRG);
Adafruit_DotStar strips[4] = {strip0, strip1, strip2, strip3};
*/

Adafruit_NeoPixel strip(NUM_OF_PIXELS, D4, NEO_GRB + NEO_KHZ800);

uint8_t ledColors[NUMBER_OF_SEGMENTS][LEDS_PER_SEGMENTS][3];  // LED buffer - each ripple writes to this, then we write this to the strips
float decay = 0.97;  // Multiply all LED's by this amount each tick to create fancy fading tails

// These ripples are endlessly reused so we don't need to do any memory management
Ripple ripples[NUMBER_OF_RIPPLES] = {
  Ripple(0),
  Ripple(1),
  Ripple(2),
  Ripple(3),
  Ripple(4),
  Ripple(5),
  Ripple(6),
  Ripple(7),
  Ripple(8),
  Ripple(9),
  Ripple(10),
  Ripple(11),
  Ripple(12),
  Ripple(13),
  Ripple(14),
  Ripple(15),
  Ripple(16),
  Ripple(17),
  Ripple(18),
  Ripple(19),
  Ripple(20),
  Ripple(21),
  Ripple(22),
  Ripple(23),
  Ripple(24),
  Ripple(25),
  Ripple(26),
  Ripple(27),
  Ripple(28),
  Ripple(29)
};


// If you don't have an EmotiBit or don't feel like wearing it, that's OK
// We'll fire automatic pulses
#define randomPulsesEnabled false  // Fire random rainbow pulses from random nodes
#define cubePulsesEnabled true  // Draw cubes at random nodes
#define starburstPulsesEnabled false  // Draw starbursts
#define simulatedBiometricsEnabled false  // Simulate heartbeat and EDA ripples

#define autoPulseTimeout 5000  // If no heartbeat is received in this many ms, begin firing random/simulated pulses
#define randomPulseTime 5000  // Fire a random pulse every (this many) ms
unsigned long lastRandomPulse;
byte lastAutoPulseNode = 255;

byte numberOfAutoPulseTypes = randomPulsesEnabled + cubePulsesEnabled + starburstPulsesEnabled;
byte currentAutoPulseType = 0;
#define autoPulseChangeTime 30000
unsigned long lastAutoPulseChange;

#define simulatedHeartbeatBaseTime 600  // Fire a simulated heartbeat pulse after at least this many ms
#define simulatedHeartbeatVariance 200  // Add random jitter to simulated heartbeat
#define simulatedEdaBaseTime 1000  // Same, but for inward EDA pulses
#define simulatedEdaVariance 10000
unsigned long nextSimulatedHeartbeat;
unsigned long nextSimulatedEda;


void renderCube(int node);
void renderUnstoppableSnake(int startingNode);
void renderContour(int node);

void setup() {
  Serial.begin(115200);
  Serial.println("*** LET'S GOOOOO ***");

  strip.begin();
  strip.show();

//  WiFi.mode(WIFI_STA);
  WiFi.begin("Nismon-IOT", "Ni$monIOT!!!!!");
  WiFi.config(ip, gateway, subnet);
  
  // Set Hostname.
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);

  // Print hostname.
  Serial.println("Hostname: " + hostname);
  //Serial.println(WiFi.hostname());


  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.print("WiFi connected, IP = ");
  Serial.println(WiFi.localIP());

  // Start OTA server.
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  // Wireless OTA updating? On an ARDUINO?! It's more likely than you think!
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  Serial.println("Ready for WiFi OTA updates");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long benchmark = millis();
  static int currentLed = 0;
  static unsigned long lastStep = 0;
  
  
  ArduinoOTA.handle();

  // Fade all dots to create trails
  for (int seg = 0; seg < NUMBER_OF_SEGMENTS; seg++) {
    for (int led = 0; led < LEDS_PER_SEGMENTS; led++) {
      for (int i = 0; i < 3; i++) {
        ledColors[seg][led][i] *= decay;
        // ledColors[seg][led][i] *= 5;
      }
    }
  }

  for (int i = 0; i < NUMBER_OF_RIPPLES; i++) {
    ripples[i].advance(ledColors);
  }

  for (uint8_t segment = 0; segment < NUMBER_OF_SEGMENTS; segment++) {
    for (uint8_t fromBottom = 0; fromBottom < LEDS_PER_SEGMENTS; fromBottom++) {
      //int strip = ledAssignments[segment][0]; // Select the proper strip number ( if you have multiple strips )
      uint16_t led = round(fmap(fromBottom,0, (LEDS_PER_SEGMENTS-1),ledAssignments[segment][2], ledAssignments[segment][1]));
      strip.setPixelColor(led, ledColors[segment][fromBottom][0], ledColors[segment][fromBottom][1],ledColors[segment][fromBottom][2]);

    }
  }
  // // strip.setPixelColor(currentLed, 0,0,0);
  // for (int i = 0; i < NUMBER_OF_SEGMENTS; i++) {

  //  strip.setPixelColor(LEDS_PER_SEGMENTS*(i-1), 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+1, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+2, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+3, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+4, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+5, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+6, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+7, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+8, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+9, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+10, 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+11, 0,0,0);
   
  //  strip.setPixelColor(LEDS_PER_SEGMENTS*i, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+1, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+2, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+3, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+4, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+5, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+6, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+7, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+8, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+9, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+10, 20,50,50);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*i)+11, 20,50,50);
  // // currentLed %= (NUMBER_OF_SEGMENTS*LEDS_PER_SEGMENTS);
  // // lastStep = millis();
  strip.show();
  
  // delay(500);
  // }


  // When biometric data is unavailable, visualize at random
  if (numberOfAutoPulseTypes && millis() - lastRandomPulse >= randomPulseTime) {
    unsigned int baseColor = random(0xFFFF);

    currentAutoPulseType = random(3);
    switch (currentAutoPulseType) {
      case 0: {
          renderContour(1);
        }

      case 1: {
          //renderCube(11);
          //renderContour(1);
          renderUnstoppableSnake(15);
          break;
        }

      case 2: {
          renderCube(random(numberOfCubeNodes));
        }

      default:
        break;
    }
    lastRandomPulse = millis();

    if (simulatedBiometricsEnabled) {
      // Simulated heartbeat
      if (millis() >= nextSimulatedHeartbeat) {
        for (int i = 0; i < SIDES_PER_NODES; i++) {
          for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
            if (ripples[j].state == STATE_DEAD) {
              ripples[j].start(
                15,
                i,
                0xEE1111,
                float(random(100)) / 100.0 * .1 + .4,
                1000,
                BEHAVIOR_WEAK);

              break;
            }
          }
        }

        nextSimulatedHeartbeat = millis() + simulatedHeartbeatBaseTime + random(simulatedHeartbeatVariance);
      }

      // Simulated EDA ripples
      if (millis() >= nextSimulatedEda) {
        for (int i = 0; i < 10; i++) {
          for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
            if (ripples[j].state == STATE_DEAD) {
              byte targetNode = borderNodes[random(numberOfBorderNodes)];
              byte direction = 255;

              while (direction == 255) {
                direction = random(6);
                if (nodeConnections[targetNode][direction] < 0)
                  direction = 255;
              }

              ripples[j].start(
                targetNode,
                direction,
                0x1111EE,
                float(random(100)) / 100.0 * .5 + 2,
                300,
                BEHAVIOR_ANGRY
              );

              break;
            }
          }
        }

        nextSimulatedEda = millis() + simulatedEdaBaseTime + random(simulatedEdaVariance);
      }
    }
  
  }
}

void renderCube(int node){

//  int node = cubeNodes[random(numberOfCubeNodes)];
  unsigned int baseColor = random(0xFFFF);
  
  while (node == lastAutoPulseNode)
    node = cubeNodes[random(numberOfCubeNodes)];

  lastAutoPulseNode = node;

  rippleBehavior behavior = random(2) ? BEHAVIOR_ALWAYS_LEFT : BEHAVIOR_ALWAYS_RIGHT;

  for (int i = 0; i < SIDES_PER_NODES; i++) {
    if (nodeConnections[node][i] >= 0) {
      for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
        if (ripples[j].state == STATE_DEAD) {
          ripples[j].start(
            node,
            i,
            strip.ColorHSV(baseColor + (0xFFFF / 6) * i, 255, 255),
            .8,
            8000,
            behavior);
          break;
        }
      }
    }
  }
}

void renderContour(int node){

//  int node = cubeNodes[random(numberOfCubeNodes)];
  unsigned int baseColor = random(0xFFFF);

  rippleBehavior behavior;

  for (int i = 0; i < SIDES_PER_NODES; i++) {
    if (nodeConnections[node][i] >= 0) {
      if ( i < 3){
        behavior = BEHAVIOR_ALWAYS_RIGHT;
      }
      else{
        behavior = BEHAVIOR_ALWAYS_LEFT;
      }
      for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
        if (ripples[j].state == STATE_DEAD) {
          ripples[j].start(
            node,
            i,
            strip.ColorHSV(baseColor + (0xFFFF / 6) * i, 255, 255),
            .8,
            8000,
            behavior);
          break;
        }
      }
    }
  }
}

void renderUnstoppableSnake(int startingNode){
//  int node = cubeNodes[random(numberOfCubeNodes)];
  unsigned int baseColor = random(0xFFFF);
  unsigned int randStartingNode = random(5);

  rippleBehavior behavior = BEHAVIOR_ANGRY;

  for (int i = 0; i < SIDES_PER_NODES; i++) {
    if (nodeConnections[startingNode][(randStartingNode+i)%5] >= 0) {
      for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
        if (ripples[j].state == STATE_DEAD) {
          ripples[j].start(
            startingNode,
            (randStartingNode+i)%5,
            strip.ColorHSV(baseColor + (0xFFFF / 6) * i, 255, 255),
            .8,
            15000,
            behavior);
          return;
        }
      }
    }
  }
}
