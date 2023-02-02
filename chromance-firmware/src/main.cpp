/*
   Chromance wall hexagon source (emotion controlled w/ EmotiBit)
   Partially cribbed from the DotStar example
   I smooshed in the ESP32 BasicOTA sketch, too

   (C) Voidstar Lab 2021
*/

#include <Arduino.h>

#include "variables.h"

#ifdef USING_DOTSTAR
#include <Adafruit_DotStar.h>
#endif
#ifdef USING_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#endif

#include <SPI.h>
#include <WiFi.h>
#include "time.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "mapping.h"
#include "ripple.h"

#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

// Function declarations
void connectToWiFi(const char *ssid, const char *pwd);
void setupOTA(void);
void continueAnimation(void);
void fade(void);
void advanceRipple(void);
void Task1code(void *pvParameters);
void getNextAnimation(void);
void loop(void);
void setup(void);
// End Function declarations

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -14400;
const int daylightOffset_sec = 3600;
// sleep 10 hours
const uint64_t TIME_TO_SLEEP = 36000;    /* Time ESP32 will go to sleep (in seconds) */
const uint64_t uS_TO_S_FACTOR = 1000000; /* Conversion factor for micro seconds to seconds */

#ifdef USING_DOTSTAR
Adafruit_DotStar strip0(BLUE_LENGTH, BLUE_STRIP_DATA_PIN, BLUE_STRIP_CLOCK_PIN, DOTSTAR_BRG);
Adafruit_DotStar strip1(GREEN_LENGTH, GREEN_STRIP_DATA_PIN, GREEN_STRIP_CLOCK_PIN, DOTSTAR_BRG);
Adafruit_DotStar strip2(RED_LENGTH, RED_STRIP_DATA_PIN, RED_STRIP_CLOCK_PIN, DOTSTAR_BRG);
Adafruit_DotStar strip3(BLACK_LENGTH, BLACK_STRIP_DATA_PIN, BLUE_STRIP_CLOCK_PIN, DOTSTAR_BRG);

Adafruit_DotStar strips[NUMBER_OF_STRIPS] = {strip0, strip1, strip2, strip3};
#endif

#ifdef USING_NEOPIXEL
Adafruit_NeoPixel strip0(BLUE_LENGTH, BLUE_STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip1(GREEN_LENGTH, GREEN_STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(RED_LENGTH, RED_STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3(BLACK_LENGTH, BLACK_STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel strips[NUMBER_OF_STRIPS] = {strip0, strip1, strip2, strip3};
#endif

// LED buffer - each ripple writes to this, then we write this to the strips
byte ledColors[NUMBER_OF_SEGMENTS][LEDS_PER_SEGMENTS][NUMBER_OF_STRIPS - 1];

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
    Ripple(29),
};

unsigned int baseColor = random(0xFFFF);

unsigned long lastRandomPulse;

byte numberOfAutoPulseTypes = randomPulsesEnabled + cubePulsesEnabled + starburstPulsesEnabled;

unsigned long nextSimulatedHeartbeat;
unsigned long nextSimulatedEda;

// Task for running on Core 0
TaskHandle_t HandleArduinoOTA_Task;

// Thread for running on opposite thread as loop
void HandleArduinoOTA(void *pvParameters)
{
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    ArduinoOTA.handle();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
      return;
    }

    if (timeinfo.tm_hour >= 22 || timeinfo.tm_hour <= 1)
    {
      esp_deep_sleep_start();
    }

    // only check for OTA update every 1/2 second
    delay(500);
  }
}

void setupOTA()
{
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
                      activeOTAUpdate= false;
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

  // loop and setup are pinned to core 1
  xTaskCreatePinnedToCore(
      HandleArduinoOTA,        /* Task function. */
      "HandleArduinoOTA_Task", /* name of task. */
      10000,                   /* Stack size of task */
      NULL,                    /* parameter of the task */
      1,                       /* priority of the task */
      &HandleArduinoOTA_Task,  /* Task handle to keep track of created task */
      0);                      /* pin task to core 0 */
}

void connectToWiFi()
{
  if (connected == true)
    return;

  WiFiManager wifiManager;

  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.autoConnect(HOSTNAME);
  Serial.printf("WiFi connected! IP address: %s", WiFi.localIP());
  connected = true;
}

void getNextAnimation()
{
  if (currentAutoPulseType == 255 || (numberOfAutoPulseTypes > 1 && millis() - lastAutoPulseChange >= RIPPES_TIMEOUT))
  {
    byte possiblePulse = 255;

    while (true)
    {
      possiblePulse = random(NUMBER_OF_ANIMATIONS);

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
}

/// Decays the color on each led
void fade()
{
  // Fade all dots to create trails
  for (int strip = 0; strip < NUMBER_OF_SEGMENTS; strip++)
  {
    for (int led = 0; led < LEDS_PER_SEGMENTS; led++)
    {
      for (int i = 0; i < (NUMBER_OF_STRIPS - 1); i++)
      {
        ledColors[strip][led][i] *= decay;
      }
    }
  }
}

// Advances the ripple forward one pixel
void advanceRipple()
{
  for (int i = 0; i < NUMBER_OF_RIPPLES; i++)
  {
    ripples[i].advance(ledColors);
  }
}

// Fades the animation
// Advances the ripple
// Updates the pixel color
void continueAnimation()
{
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
}

u_int32_t getRandomColor()
{
  return strip0.ColorHSV(baseColor, 255, 255);
}

void randomPulse()
{
  int node = funNodes[random(numberOfFunNodes)];
  while (node == lastAutoPulseNode)
    node = funNodes[random(numberOfFunNodes)];

  lastAutoPulseNode = node;

  for (int i = 0; i < MAX_PATHS_PER_NODES; i++)
  {
    if (nodeConnections[node][i] >= 0)
    {
      for (int j = 0; j < NUMBER_OF_RIPPLES; j++)
      {
        if (ripples[j].state == STATE_DEAD)
        {
          ripples[j].start(
              node,
              i,
              getRandomColor(),
              float(random(100)) / 100.0 * .2 + .5,
              ANIMATION_TIME,
              BEHAVIOR_FEISTY);

          break;
        }
      }
    }
  }
}

void cubePulse()
{
  int node = cubeNodes[random(numberOfCubeNodes)];

  while (node == lastAutoPulseNode)
    node = cubeNodes[random(numberOfCubeNodes)];

  lastAutoPulseNode = node;

  rippleBehavior behavior = random(2) ? BEHAVIOR_ALWAYS_LEFT : BEHAVIOR_ALWAYS_RIGHT;

  for (int i = 0; i < MAX_PATHS_PER_NODES; i++)
  {
    if (nodeConnections[node][i] >= 0)
    {
      for (int j = 0; j < NUMBER_OF_RIPPLES; j++)
      {
        if (ripples[j].state == STATE_DEAD)
        {
          ripples[j].start(
              node,
              i,
              getRandomColor(),
              .8,
              ANIMATION_TIME,
              behavior);

          break;
        }
      }
    }
  }
}

void starburstPulse()
{
  rippleBehavior behavior = random(2) ? BEHAVIOR_ALWAYS_LEFT : BEHAVIOR_ALWAYS_RIGHT;

  lastAutoPulseNode = starburstNode;

  for (int i = 0; i < MAX_PATHS_PER_NODES; i++)
  {
    for (int j = 0; j < NUMBER_OF_RIPPLES; j++)
    {
      if (ripples[j].state == STATE_DEAD)
      {
        ripples[j].start(
            starburstNode,
            i,
            strip0.ColorHSV(baseColor + (0xFFFF / 6) * i, 255, 255),
            .65,
            2600,
            behavior);

        break;
      }
    }
  }
}

void startAnimation(byte animation)
{
  switch (currentAutoPulseType)
  {
    // RANDOM PULSES
  case 0:
  {
    randomPulse();
    break;
  }

  // cubePulsesEnabled
  case 1:
  {
    cubePulse();
    break;
  }

  // starburstPulsesEnabled
  case 2:
  {
    starburstPulse();
    break;
  }

  default:
    break;
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
  setupOTA();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

void loop()
{
  // We are doing an OTA update, might as well just stop
  if (activeOTAUpdate)
  {
    return;
  }

  continueAnimation();

  if (numberOfAutoPulseTypes && millis() - lastRandomPulse >= randomPulseTime)
  {
    // float lastColor = (float)baseColor / (float)0xFFFF * 360;
    // while (true)
    // {
    baseColor = random(0xFFFF);
    //   float currentColor = baseColor / 65535 * 360;
    //   int hueDistance = min(abs(lastColor - currentColor), 360 - abs(lastColor - currentColor));
    //   if (hueDistance > 20)
    //     break;
    // }

    // change animation every 30 seconds
    getNextAnimation();

    startAnimation(currentAutoPulseType);
    lastRandomPulse = millis();
  }
}
