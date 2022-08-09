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
#include <Stream.h>
#include "debugUdp.h"

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
unsigned int debugPort = 8000;      // local port to listen on

WiFiUDP Udp;

Adafruit_NeoPixel strip(NUM_OF_PIXELS, D4, NEO_GRB + NEO_KHZ800);

uint8_t ledColors[NUMBER_OF_SEGMENTS][LEDS_PER_SEGMENTS][3];  // LED buffer - each ripple writes to this, then we write this to the strips
float decay = 0.97;  // Multiply all LED's by this amount each tick to create fancy fading tails

uint16_t g_hue = 40000;
uint32_t g_color;
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
void setFixedColor(uint32_t color);

ESP8266WebServer server(80);
debugUdp dbg(8000);

String page = MAIN_page;

// Handle Web Requests

void handleRoot() {
    dbg.print("Page opened");
    server.send(200, "text/html", page);
}

void setCurrentMode(int mode){
    currentMode = mode;
}

void setUDPMode(){
    setCurrentMode(1);
    dbg.println("Setting to UDP listener ");
    clearLeds();
    server.send(200, "text/html", page);
}

void setChromanceMode(){
    setCurrentMode(0);
    dbg.println("Setting to chromance mode ");
    clearLeds();
    server.send(200, "text/html", page);
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
        dbg.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        dbg.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        dbg.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        dbg.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) dbg.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) dbg.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) dbg.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) dbg.println("Receive Failed");
        else if (error == OTA_END_ERROR) dbg.println("End Failed");
    });

    ArduinoOTA.begin();
}

void setupHTTPServer(){
    server.on("/", handleRoot);

    server.on("/mode/set_hue", set_hue);
    server.on("/mode/chromance", setChromanceMode);
    server.on("/mode/udp", setUDPMode);
    server.on("/inline", []() {
        server.send(200, "text/plain", "this works as well");
    });

    server.onNotFound(handleNotFound);
    server.begin();
}

void clearLeds(){
    memset(ledColors,0, NUMBER_OF_SEGMENTS*LEDS_PER_SEGMENTS*3);
    strip.clear();
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


    //dbg.println(WiFi.hostname());

    
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    Udp.begin(localPort);

    dbg.write("Online");
    // Print hostname.
    dbg.println("Hostname: " + hostname);
    dbg.print("WiFi connected, IP = ");
    dbg.println(WiFi.localIP());

    setupOTA();

    dbg.println("Ready for WiFi OTA updates");
    dbg.print("IP address: ");
    dbg.println(WiFi.localIP());


    setupHTTPServer();
    setFixedColor(14230237);
}

void loop() {
    unsigned long benchmark = millis();
    static int currentLed = 0;
    static unsigned long lastStep = 0;
    
    
    ArduinoOTA.handle();
    server.handleClient();


    // if ( currentMode == 0 ){
    //     chromanceProcess();
    // }else{
    //     processUDP();
    // }

    // //
    // for (uint8_t segment = 0; segment < NUMBER_OF_SEGMENTS; segment++) {
    //     for (uint8_t fromBottom = 0; fromBottom < LEDS_PER_SEGMENTS; fromBottom++) {

    //         uint16_t led = round(fmap(fromBottom,0, (LEDS_PER_SEGMENTS-1),ledAssignments[segment][2], ledAssignments[segment][1]));
    //         strip.setPixelColor(led, ledColors[segment][fromBottom][0], ledColors[segment][fromBottom][1],ledColors[segment][fromBottom][2]);
    //     }
    // }
    // // Update LEDS !
    // strip.show();

}



void setFixedColor(uint32_t color){

    g_color  = color;
    uint8_t r = (g_color & 0xff0000 ) >> 16;
    uint8_t g = (g_color & 0x00ff00 ) >> 8;
    uint8_t b = (g_color & 0x0000ff ) >> 0;

    for (int i = 0; i < NUM_OF_PIXELS; i++) {
        strip.setPixelColor(i, r,g,b);
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
        dbg.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
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
        // dbg.println("Contents:");
        // dbg.println(packetBuffer);

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
        dbg.write("Alive");
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

    rippleBehavior behavior = BEHAVIOR_ANGRY;
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
//   uint16_t color = secureRandom(765,1530);
    return strip.ColorHSV(g_hue, random(200,255), random(100,255));
}

float getSpeed(){
    return random(500,800)/1000.0;
  // return 0.5;
}

void set_hue(){
    
    if (server.args() > 0){
        for (uint8_t i = 0; i < server.args(); i++) {
                if (server.argName(i) == "hue"){
                    String hue_arg = server.arg(i);
                    
                    if (hue_arg.charAt(0) == '#'){
                        hue_arg.setCharAt(0,'0');
                    }
                    setFixedColor(strtol(hue_arg.c_str(), 0, 16));
                    Serial.println("Setting hue to : ");
                    Serial.print(strtol(hue_arg.c_str(), 0, 16));
                }
                Serial.println("Arg : ");
                Serial.print(server.arg(i));
                server.send(200, "text/plain", "Setting hue to " + server.arg(i));
            }
    }   
}

void testStrip(){
    // // strip.setPixelColor(currentLed, 0,0,0);
  // for (int i = 0; i < NUMBER_OF_SEGMENTS; i++) {

  //  strip.setPixelColor(LEDS_PER_SEGMENTS*(i-1), 0,0,0);
  //  strip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+1, 0,0,0);
  //  stfgnvb nrip.setPixelColor((LEDS_PER_SEGMENTS*(i-1))+2, 0,0,0);
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