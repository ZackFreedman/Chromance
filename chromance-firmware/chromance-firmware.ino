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
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "mapping.h"
#include "ripple.h"
#include "index.h"

#define HOSTNAME "ESP8266-OTA-" ///< Hostname. The setup function adds the Chip ID at the end.
#define NUMBER_OF_RIPPLES 30
// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const IPAddress ip(192, 168, 5, 50);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 5, 1);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)
const int port = 23;

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 2

unsigned int localPort = 8888;      // local port to listen on


WiFiUDP Udp;

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

byte lastAutoPulseNode = 255;

byte currentAutoPulseType = 0;
#define autoPulseChangeTime 30000
unsigned long lastAutoPulseChange;

#define simulatedHeartbeatBaseTime 600  // Fire a simulated heartbeat pulse after at least this many ms
#define simulatedHeartbeatVariance 200  // Add random jitter to simulated heartbeat
#define simulatedEdaBaseTime 1000  // Same, but for inward EDA pulses
#define simulatedEdaVariance 10000
unsigned long nextSimulatedHeartbeat;
unsigned long nextSimulatedEda;
int currentMode = 0;

void renderCube(int node);
void renderUnstoppableSnake(int startingNode);
void renderContour(int node);
void renderStarburst(void);

// Process the original chromance animation
void chromanceProcess(void);
void processUDP(void);

ESP8266WebServer server(80);
String page = MAIN_page;

void handleRoot() {

    server.send(200, "text/html", page);
}

void setCurrentMode(int mode){
    currentMode = mode;
}

void setUDPMode(){
    setCurrentMode(1);
    clearLeds();
    server.send(200, "text/html", page);
}

void setChromanceMode(){
    setCurrentMode(0);
    clearLeds();
    server.send(200, "text/html", page);
}

void clearLeds(){
    memset(ledColors,0, NUMBER_OF_SEGMENTS*LEDS_PER_SEGMENTS*3);
    strip.clear();
}

void handleNotFound() {
    // digitalWrite(led, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    // digitalWrite(led, 0);
    }

void setupOTA(){
    String hostname(HOSTNAME);
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
}

void setupHTTPServer(){
    server.on("/", handleRoot);

    server.on("/mode/chromance", setChromanceMode);
    server.on("/mode/udp", setUDPMode);
    server.on("/inline", []() {
        server.send(200, "text/plain", "this works as well");
    });

    server.on("/gif", []() {
        static const uint8_t gif[] PROGMEM = {
        0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x10, 0x00, 0x10, 0x00, 0x80, 0x01,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x10, 0x00, 0x00, 0x02, 0x19, 0x8c, 0x8f, 0xa9, 0xcb, 0x9d,
        0x00, 0x5f, 0x74, 0xb4, 0x56, 0xb0, 0xb0, 0xd2, 0xf2, 0x35, 0x1e, 0x4c,
        0x0c, 0x24, 0x5a, 0xe6, 0x89, 0xa6, 0x4d, 0x01, 0x00, 0x3b
        };
        char gif_colored[sizeof(gif)];
        memcpy_P(gif_colored, gif, sizeof(gif));
        // Set the background to a random set of colors
        gif_colored[16] = millis() % 256;
        gif_colored[17] = millis() % 256;
        gif_colored[18] = millis() % 256;
        server.send(200, "image/gif", gif_colored, sizeof(gif_colored));
    });

    server.onNotFound(handleNotFound);
    server.begin();
}

// Main setup function
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

    setupOTA();

    Serial.println("Ready for WiFi OTA updates");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    Udp.begin(localPort);

    setupHTTPServer();
}

void loop() {
    unsigned long benchmark = millis();
    static int currentLed = 0;
    static unsigned long lastStep = 0;
    
    
    ArduinoOTA.handle();
    server.handleClient();


    if ( currentMode == 0 ){
        chromanceProcess();
    }else{
        processUDP();
    }

    //
    for (uint8_t segment = 0; segment < NUMBER_OF_SEGMENTS; segment++) {
        for (uint8_t fromBottom = 0; fromBottom < LEDS_PER_SEGMENTS; fromBottom++) {

            uint16_t led = round(fmap(fromBottom,0, (LEDS_PER_SEGMENTS-1),ledAssignments[segment][2], ledAssignments[segment][1]));
            strip.setPixelColor(led, ledColors[segment][fromBottom][0], ledColors[segment][fromBottom][1],ledColors[segment][fromBottom][2]);

        }
    }

    // Update LEDS !
    strip.show();

}

void processUDP(){
    // buffers for receiving and sending data
    static char  ReplyBuffer[] = "acknowledged\r\n";       // a string to send back
    static char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,

      // if there's data available, read a packet
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
                        packetSize,
                        Udp.remoteIP().toString().c_str(), Udp.remotePort(),
                        Udp.destinationIP().toString().c_str(), Udp.localPort(),
                        ESP.getFreeHeap());

        // read the packet into packetBufffer
        int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        
        
        if (n >= LEDS_PER_SEGMENTS*NUMBER_OF_SEGMENTS*3){
            memcpy(ledColors, packetBuffer, LEDS_PER_SEGMENTS*NUMBER_OF_SEGMENTS*3);
        }
        else{
            packetBuffer[n] = 0;
            switch (packetBuffer[0]){

                case 's':
                    renderStarburst();
                    break;
                case 'c':
                    renderCube(random(numberOfCubeNodes));
                    break;
                case 'o':
                    renderContour(1);
                    break;
                case 't':
                    test();
                    break;
            }
        }
        // Serial.println("Contents:");
        // Serial.println(packetBuffer);

        // send a reply, to the IP address and port that sent us the packet we received
        // Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        // Udp.write(ReplyBuffer);
        // Udp.endPacket();
    }
}

void chromanceProcess(){
    static unsigned long lastRandomPulse;
    
    // Fade all dots to create trails
    for (int seg = 0; seg < NUMBER_OF_SEGMENTS; seg++) {
        for (int led = 0; led < LEDS_PER_SEGMENTS; led++) {
            for (int i = 0; i < 3; i++) {
                ledColors[seg][led][i] *= decay;
            }
        }
    }

    // Advance each started ripple
    for (int i = 0; i < NUMBER_OF_RIPPLES; i++) {
        ripples[i].advance(ledColors);
    }

    if (millis() - lastRandomPulse >= random(2000,6000)) {

        // renderTriburst();
        
        currentAutoPulseType = random(5);
        switch (currentAutoPulseType) {
        case 0: {
            renderContour(1);
            break;
            }

        case 1: {
            renderUnstoppableSnake(random(0,NUMBER_OF_NODES));
            break;
            }

        case 2: {
            renderCube(random(numberOfCubeNodes));
            break;
            }

        case 3: {
            renderStarburst();
            break;
            }
        case 4: {
            renderTriburst();
            break;
            }

        default:
            break;
        }
        
        lastRandomPulse = millis(); 
    }
}

void renderCube(int node){

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
                getRandomColor(),
                getSpeed(),
                8000,
                behavior);
            break;
            }
        }
        }
    }
}

void renderContour(int node){

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
                getRandomColor(),
                getSpeed(),
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
    unsigned int randStartingNode = random(5);

    rippleBehavior behavior = BEHAVIOR_ANGRY;

    for (int i = 0; i < SIDES_PER_NODES; i++) {
        if (nodeConnections[startingNode][(randStartingNode+i)%5] >= 0) {
        for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
            if (ripples[j].state == STATE_DEAD) {
            ripples[j].start(
                startingNode,
                (randStartingNode+i)%5,
                getRandomColor(),
                getSpeed(),
                8000,
                behavior);
            return;
            }
        }
        }
    }
}

void renderStarburst(){
    //  int node = cubeNodes[random(numberOfCubeNodes)];
    unsigned int startingNode = starburstNode;

    rippleBehavior behavior = BEHAVIOR_FEISTY;
    uint32_t color = getRandomColor();
    for (int i = 0; i < SIDES_PER_NODES; i++) {
        if (nodeConnections[startingNode][i] >= 0) {
        for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
            if (ripples[j].state == STATE_DEAD) {
            ripples[j].start(
                startingNode,
                i,
                getRandomColor(),
                getSpeed(),
                5000,
                behavior);
            break;
            }
        }
        }
    }
}

void renderTriburst(){
    //  int node = cubeNodes[random(numberOfCubeNodes)];
    unsigned int startingNode = triNodes[random(numberOfTriNodes)];;

    rippleBehavior behavior = BEHAVIOR_COUCH_POTATO;
    uint8_t rippleCnt = 0;
    int i=0;
    
    while (rippleCnt < 3) {
        if (nodeConnections[startingNode][i] >= 0) {
        i+=2; // Skip 1 side
        i%=6;
        rippleCnt ++;
        for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
            if (ripples[j].state == STATE_DEAD) {
            ripples[j].start(
                startingNode,
                i,
                getRandomColor(),
                getSpeed(),
                random(500,2500),
                behavior);
            break;
            }
        }
        }else{
        i++;
        }
    }
}


void test(){
//  int node = cubeNodes[random(numberOfCubeNodes)];
  unsigned int startingNode = starburstNode;
  static int node = 0;

  rippleBehavior behavior = BEHAVIOR_LAZY;
  if (nodeConnections[startingNode][node] >= 0) {
    for (int j = 0; j < NUMBER_OF_RIPPLES; j++) {
      if (ripples[j].state == STATE_DEAD) {
        ripples[j].start(
          startingNode,
          node,
          getRandomColor(),
          getSpeed(),
          5000,
          behavior);
        break;
      }
    }
  }
  node++;
  node %= 6;
}

uint32_t getRandomColor()
{
  uint16_t color = random(0x7FFF,0xFFFF);
  return strip.ColorHSV(color, random(200,255), random(100,255));
}

float getSpeed(){
  return random(500,800)/1000.0;
  // return 0.5;
}

void testStrip(){
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
}