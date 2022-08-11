/*
This example will receive multiple universes via Art-Net and control a strip of
WS2812 LEDs via the FastLED library: https://github.com/FastLED/FastLED
This example may be copied under the terms of the MIT license, see the LICENSE file for details
*/
#include <ArtnetWifi.h>
#include <Arduino.h>
#include <FastLED.h>

// Wifi settings
#include "wifi.h"

// LED settings
const int numLeds = 12; // CHANGE FOR YOUR SETUP
const int numberOfChannels = numLeds * 3 * 2 + 2; // Total number of channels you want to receive (1 led = 3 channels)
const byte dataPin1 = 16;
const byte dataPin2 = 17;
CRGB leds[numLeds];
CRGB leds2[numLeds];

// Art-Net settings
ArtnetWifi artnet;
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.

// Check if we got all universes
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;


// connect to wifi – returns true if successful or false if not
bool ConnectWifi(void)
{
  bool state = true;
  int i = 0;

  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (i > 20)
    {
      state = false;
      break;
    }
    i++;
  }
  if (state)
  {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("");
    Serial.println("Connection failed.");
  }

  return state;
}

void initTest()
{
  for (int i = 0 ; i < numLeds ; i++)
  {
    leds[i] = CRGB(127, 0, 0);
    leds2[i] = CRGB(127, 0, 0);    
  }
  FastLED.show();
  delay(500);
  for (int i = 0 ; i < numLeds ; i++)
  {
    leds[i] = CRGB(0, 127, 0);
    leds2[i] = CRGB(0, 127, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0 ; i < numLeds ; i++)
  {
    leds[i] = CRGB(0, 0, 127);
    leds2[i] = CRGB(0, 0, 127);
  }
  FastLED.show();
  delay(500);
  for (int i = 0 ; i < numLeds ; i++)
  {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}


uint8_t DMXdata[numberOfChannels];


void rainbow(uint8_t startHue)
{
  uint8_t hueStep = 255/numLeds;
  uint8_t hue = startHue;
  for(int i=0; i < numLeds; i++){
    hue += hueStep;
    leds[i] = CHSV(hue, 255, 255);   
    leds2[i] = CHSV(hue, 255, 255);   
  }
}

void setPixelFromDmx(int loopIndex) {
  for (int i = 0; i < (numberOfChannels-2)/ 3; i++)
  {
    int led = (i + loopIndex)%numLeds;
    if (i < numLeds)
    {
        leds[led] = CRGB(DMXdata[i * 3], DMXdata[i * 3 + 1], DMXdata[i * 3 + 2]);
        //Serial.println(led);
        //Serial.println(data[i*3]);
    } else if(i < (numLeds*2)){
        leds2[led] = CRGB(DMXdata[i * 3], DMXdata[i * 3 + 1], DMXdata[i * 3 + 2]);
     
    }
  }    
}
void setColourFromDmx(int loopIndex) {
  for (int i = 0; i < numLeds; i++)
  {
    leds[i] = CRGB(DMXdata[loopIndex * 3], DMXdata[loopIndex * 3 + 1], DMXdata[loopIndex * 3 + 2]);
    leds2[i] = CRGB(DMXdata[(loopIndex+numLeds) * 3], DMXdata[(loopIndex+numLeds) * 3 + 1], DMXdata[(loopIndex+numLeds) * 3 + 2]);
  }
}


uint8_t loopLocation = 0;

void applyLEDs()
{
  loopLocation = (loopLocation + 1) % 252;
  // set brightness of the whole strip
  FastLED.setBrightness(3);
  FastLED.show();
  if(DMXdata[numberOfChannels-2] < 5) {
    // read data from dmx channels
    setPixelFromDmx(0);
  } else {
    // custom effects
    int effectSpeed = 255-DMXdata[numberOfChannels-1];
    delay(effectSpeed*4);
    if(DMXdata[numberOfChannels-2] < 10){
      setPixelFromDmx(loopLocation%numLeds);
    } else if(DMXdata[numberOfChannels-2] < 15)
    {
      rainbow(loopLocation*8);
    } else if(DMXdata[numberOfChannels-2] < 20)
    {
      setColourFromDmx(loopLocation%numLeds);
    } else 
    {
      rainbow(loopLocation*8);
    }
  }
  FastLED.show();
  // Reset universeReceived to 0

}



void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  int sendFrame=1;
  // range check
  if (universe < startUniverse)
  {
    return;
  }
  uint8_t index = universe - startUniverse;
  if (index >= maxUniverses)
  {
    return;
  }

  // Store which universe has got in
  universesReceived[index] = true;

  for (int i = 0 ; i < maxUniverses ; i++)
  {
    if (!universesReceived[i])
    {
      sendFrame = 0;
      break;
    }
  }

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < length; i++)
  {
    if(i < numberOfChannels) {
      DMXdata[i] = data[i];
    }
  }

  if (sendFrame)
  {
    memset(universesReceived, 0, maxUniverses);
  }
}

void setup()
{
  Serial.begin(115200);
  ConnectWifi();
  artnet.begin();
  FastLED.addLeds<WS2812, dataPin1, GRB>(leds, numLeds);
  FastLED.addLeds<WS2812, dataPin2, GRB>(leds2, numLeds);
  initTest();
  for (int i = 0; i < numberOfChannels; i++)
  {
    DMXdata[i] = 0;
  }
  DMXdata[numberOfChannels-2] = 10; //rainbow
  DMXdata[numberOfChannels-1] = 58; //medium speed
  


  memset(universesReceived, 0, maxUniverses);
  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);
}

void loop()
{
  // we call the read function inside the loop
  artnet.read();
  applyLEDs();
}
