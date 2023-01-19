/*
   Chromance wall hexagon source (emotion controlled w/ EmotiBit)
   Partially cribbed from the DotStar example
   I smooshed in the ESP32 BasicOTA sketch, too

   (C) Voidstar Lab 2021
*/

// TODO: USER SET
// COMMENT OUT IF NOT USING
// #define USING_EMOTI_BIT_SENSOR

#include <Arduino.h>

#include "variables.h"

#ifdef USING_DOTSTAR
#include <Adafruit_DotStar.h>
#endif
#ifdef USING_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#endif

#include <SPI.h>
#ifdef USING_EMOTI_BIT_SENSOR
#include <ArduinoOSCWiFi.h>
#endif
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "mapping.h"
#include "ripple.h"

#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

void connectToWiFi(const char *ssid, const char *pwd);
void setupOTA(void);
void fade(void);
void advanceRipple(void);
void Task1code(void *pvParameters);

#ifdef USING_DOTSTAR
Adafruit_DotStar strip0(BLUE_LENGTH, BLUE_STRIP_DATA_PIN, BLUE_STRIP_CLOCK_PIN, DOTSTAR_BRG);
Adafruit_DotStar strip1(GREEN_LENGTH, GREEN_STRIP_DATA_PIN, GREEN_STRIP_CLOCK_PIN, DOTSTAR_BRG);
Adafruit_DotStar strip2(RED_LENGTH, RED_STRIP_DATA_PIN, RED_STRIP_CLOCK_PIN, DOTSTAR_BRG);
Adafruit_DotStar strip3(BLACK_LENGTH, BLACK_STRIP_DATA_PIN, BLUE_STRIP_CLOCK_PIN, DOTSTAR_BRG);

Adafruit_DotStar strips[4] = {strip0, strip1, strip2, strip3};
#endif

#ifdef USING_NEOPIXEL
Adafruit_NeoPixel strip0(BLUE_LENGTH, BLUE_STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip1(GREEN_LENGTH, GREEN_STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(RED_LENGTH, RED_STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3(BLACK_LENGTH, BLACK_STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel strips[NUMBER_OF_STRIPS] = {strip0, strip1, strip2, strip3};
#endif

byte ledColors[NUMBER_OF_SEGMENTS][LEDS_PER_SEGMENTS][NUMBER_OF_STRIPS - 1]; // LED buffer - each ripple writes to this, then we write this to the strips

// These ripples are endlessly reused so we don't need to do any memory management
#define numberOfRipples 30
Ripple ripples[numberOfRipples] = {
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
    Ripple(29),
};

// Biometric detection and interpretation
// IR (heartbeat) is used to fire outward ripples
float lastIrReading;         // When our heart pumps, reflected IR drops sharply
float highestIrReading;      // These vars let us detect this drop
unsigned long lastHeartbeat; // Track last heartbeat so we can detect noise/disconnections
#define heartbeatLockout 500 // Heartbeats that happen within this many milliseconds are ignored
#define heartbeatDelta 300   // Drop in reflected IR that constitutes a heartbeat

// Heartbeat color ripples are proportional to skin temperature
#define lowTemperature 33.0                                            // Resting temperature
#define highTemperature 37.0                                           // Really fired up
float lastKnownTemperature = (lowTemperature + highTemperature) / 2.0; // Carries skin temperature from temperature callback to IR callback

// EDA code was too unreliable and was cut.
// TODO: Rebuild EDA code

// Gyroscope is used to reject data if you're moving too much
#define gyroAlpha 0.9     // Exponential smoothing constant
#define gyroThreshold 300 // Minimum angular velocity total (X+Y+Z) that disqualifies readings
float gyroX, gyroY, gyroZ;

unsigned long lastRandomPulse;

byte numberOfAutoPulseTypes = randomPulsesEnabled + cubePulsesEnabled + starburstPulsesEnabled;

unsigned long nextSimulatedHeartbeat;
unsigned long nextSimulatedEda;

// Task for running on Core 0
TaskHandle_t Task1;

void setupOTA()
{
  String hostname(HOSTNAME);
  // Start OTA server.
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword("esp32password");
  // Wireless OTA updating? On an ARDUINO?! It's more likely than you think!
  ArduinoOTA
      .onStart([]()
               {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    
    activeOTAUpdate= true;
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                         Serial.println("Auth Failed");
                       else if (error == OTA_BEGIN_ERROR)
                         Serial.println("Begin Failed");
                       else if (error == OTA_CONNECT_ERROR)
                         Serial.println("Connect Failed");
                       else if (error == OTA_RECEIVE_ERROR)
                         Serial.println("Receive Failed");
                       else if (error == OTA_END_ERROR)
                         Serial.println("End Failed"); });

  ArduinoOTA.begin();
}

void connectToWiFi()
{
  if (connected == true)
    return;

  WiFiManager wifiManager;

  wifiManager.autoConnect(HOSTNAME);

  Serial.printf("Hostname: %s", HOSTNAME);
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());
  setupOTA();
  connected = true;
}

// Thread for running on opposite thread as loop
void Task1code(void *pvParameters)
{
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    ArduinoOTA.handle();
    // only check for OTA update every 1/2 second
    delay(500);
  }
}

void setup()
{
  Serial.begin(115200);

  Serial.println("*** LET'S GOOOOO ***");

  for (int i = 0; i < NUMBER_OF_STRIPS; i++)
  {
    strips[i].begin();
    strips[i].setBrightness(255); // If your PSU sucks, use this to limit the current
    strips[i].show();
  }

  connectToWiFi();

  // loop and setup are pinned to core 1
  xTaskCreatePinnedToCore(
      Task1code, /* Task function. */
      "Task1",   /* name of task. */
      10000,     /* Stack size of task */
      NULL,      /* parameter of the task */
      1,         /* priority of the task */
      &Task1,    /* Task handle to keep track of created task */
      0);        /* pin task to core 0 */
}

void fade()
{
  // Fade all dots to create trails
  for (int strip = 0; strip < 40; strip++)
  {
    for (int led = 0; led < 14; led++)
    {
      for (int i = 0; i < 3; i++)
      {
        ledColors[strip][led][i] *= decay;
      }
    }
  }
}

void advanceRipple()
{
  for (int i = 0; i < numberOfRipples; i++)
  {
    ripples[i].advance(ledColors);
  }
}

void loop()
{
#ifdef USING_EMOTI_BIT_SENSOR
  OscWiFi.parse();
#endif

  // We are doing an OTA update, might as well just stop
  if (activeOTAUpdate)
  {
    return;
  }

  fade();

  advanceRipple();

  for (int segment = 0; segment < NUMBER_OF_SEGMENTS; segment++)
  {
    for (int fromBottom = 0; fromBottom < LEDS_PER_SEGMENTS; fromBottom++)
    {
      int strip = ledAssignments[segment][0];
      int led = round(fmap(
          fromBottom,
          0, (LEDS_PER_SEGMENTS - 1),
          ledAssignments[segment][2], ledAssignments[segment][1]));
      strips[strip].setPixelColor(
          led,
          ledColors[segment][fromBottom][0],
          ledColors[segment][fromBottom][1],
          ledColors[segment][fromBottom][2]);
    }
  }

  for (int i = 0; i < NUMBER_OF_STRIPS; i++)
    strips[i].show();

#ifdef USING_EMOTI_BIT_SENSOR
  if (millis() - lastHeartbeat >= autoPulseTimeout)
  {
#endif
    // When biometric data is unavailable, visualize at random
    if (numberOfAutoPulseTypes && millis() - lastRandomPulse >= randomPulseTime)
    {
      unsigned int baseColor = random(0xFFFF);

      if (currentAutoPulseType == 255 || (numberOfAutoPulseTypes > 1 && millis() - lastAutoPulseChange >= autoPulseChangeTime))
      {
        byte possiblePulse = 255;
        while (true)
        {
          possiblePulse = random(3);

          if (possiblePulse == currentAutoPulseType)
            continue;

          switch (possiblePulse)
          {
          case 0:
            if (!randomPulsesEnabled)
              continue;
            break;

          case 1:
            if (!cubePulsesEnabled)
              continue;
            break;

          case 2:
            if (!starburstPulsesEnabled)
              continue;
            break;

          default:
            continue;
          }

          currentAutoPulseType = possiblePulse;
          lastAutoPulseChange = millis();
          break;
        }
      }

      switch (currentAutoPulseType)
      {
        // RANDOM PULSES
      case 0:
      {
        int node = 0;
        bool foundStartingNode = false;

        while (!foundStartingNode)
        {
          node = random(25);
          foundStartingNode = true;
          for (int i = 0; i < numberOfBorderNodes; i++)
          {
            // Don't fire a pulse on one of the outer nodes - it looks boring
            if (node == borderNodes[i])
              foundStartingNode = false;
          }

          if (node == lastAutoPulseNode)
            foundStartingNode = false;
        }

        lastAutoPulseNode = node;

        for (int i = 0; i < MAX_SIDES_PER_NODES; i++)
        {
          if (nodeConnections[node][i] >= 0)
          {
            for (int j = 0; j < numberOfRipples; j++)
            {
              if (ripples[j].state == STATE_DEAD)
              {
                ripples[j].start(
                    node,
                    i,
                    strip0.ColorHSV(baseColor, 255, 255),
                    float(random(100)) / 100.0 * .2 + .5,
                    3000,
                    BEHAVIOR_FEISTY);

                break;
              }
            }
          }
        }
        break;
      }

      // cubePulsesEnabled
      case 1:
      {
        int node = cubeNodes[random(numberOfCubeNodes)];

        while (node == lastAutoPulseNode)
          node = cubeNodes[random(numberOfCubeNodes)];

        lastAutoPulseNode = node;

        rippleBehavior behavior = random(2) ? BEHAVIOR_ALWAYS_LEFT : BEHAVIOR_ALWAYS_RIGHT;

        for (int i = 0; i < MAX_SIDES_PER_NODES; i++)
        {
          if (nodeConnections[node][i] >= 0)
          {
            for (int j = 0; j < numberOfRipples; j++)
            {
              if (ripples[j].state == STATE_DEAD)
              {
                ripples[j].start(
                    node,
                    i,
                    strip0.ColorHSV(baseColor, 255, 255),
                    .8,
                    3000,
                    behavior);

                break;
              }
            }
          }
        }
        break;
      }

      // starburstPulsesEnabled
      case 2:
      {
        rippleBehavior behavior = random(2) ? BEHAVIOR_ALWAYS_LEFT : BEHAVIOR_ALWAYS_RIGHT;

        lastAutoPulseNode = starburstNode;

        for (int i = 0; i < MAX_SIDES_PER_NODES; i++)
        {
          for (int j = 0; j < numberOfRipples; j++)
          {
            if (ripples[j].state == STATE_DEAD)
            {
              ripples[j].start(
                  starburstNode,
                  i,
                  strip0.ColorHSV(baseColor + (0xFFFF / 6) * i, 255, 255),
                  .65,
                  2500,
                  behavior);

              break;
            }
          }
        }
        break;
      }

      default:
        break;
      }
      lastRandomPulse = millis();
    }

    if (simulatedBiometricsEnabled)
    {
      // Simulated heartbeat
      if (millis() >= nextSimulatedHeartbeat)
      {
        for (int i = 0; i < 6; i++)
        {
          for (int j = 0; j < numberOfRipples; j++)
          {
            if (ripples[j].state == STATE_DEAD)
            {
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
      if (millis() >= nextSimulatedEda)
      {
        for (int i = 0; i < 10; i++)
        {
          for (int j = 0; j < numberOfRipples; j++)
          {
            if (ripples[j].state == STATE_DEAD)
            {
              byte targetNode = borderNodes[random(numberOfBorderNodes)];
              byte direction = 255;

              while (direction == 255)
              {
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
                  BEHAVIOR_ANGRY);

              break;
            }
          }
        }

        nextSimulatedEda = millis() + simulatedEdaBaseTime + random(simulatedEdaVariance);
      }
    }
#ifdef USING_EMOTI_BIT_SENSOR
  }
#endif
}
