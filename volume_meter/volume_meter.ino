// Code borrowed heavily from Adafruit
// https://learn.adafruit.com/gemma-powered-neopixel-led-sound-reactive-drums?view=all

#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 6  // NeoPixel data out pin
#define MIC_PIN 0       // Analog pin mic connected to
#define NUM_PIXELS 60   // Length of NeoPixel strip
#define NUM_SAMPLES 60  // Number of mic samples to keep in buffer
#define DC_OFFSET 200   // Offset in mic signal - sets tolerance
#define NOISE 200       // Noise/hum/interference in mic signal
#define TOP (NUM_PIXELS + 1) // Allow dot to go slightly off scale
#define BRIGHTNESS 60   // General NeoPixel brightness (max 255)
#define SPEED .20       // Amount to increment RGB color by each cycle

byte
  peak = 0,
  dotCount = 0,
  volCount = 0;

int
  vol[NUM_SAMPLES],
  lvl = 10,
  minLvlAvg = 0,
  maxLvlAvg = 512;

float
  greenOffset = 30,
  blueOffset = 150;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(
  NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800
);

void setup() {
  memset(vol, 0, sizeof(vol));
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(BRIGHTNESS);
}

void loop() {
  uint8_t i;
  uint16_t minLvl, maxLvl;
  int n, height;

  n = analogRead(MIC_PIN);             // Raw reading from mic
  n = abs(n - 512 - DC_OFFSET);        // Center on zero
  n = (n <= NOISE) ? 0 : (n - NOISE);  // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;      // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top

  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for (i = 0; i < NUM_PIXELS; i++) {
    if (i >= height) {
      strip.setPixelColor(i, 0, 0, 0);
    } else {
      strip.setPixelColor(i, Wheel(
        map(i, 0, strip.numPixels() - 1, (int)greenOffset, (int)blueOffset)
      ));
    }
  }

  strip.show();  // Update strip

  vol[volCount] = n;
  if (++volCount >= NUM_SAMPLES) {
    volCount = 0;
  }

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < NUM_SAMPLES; i++) {
    if (vol[i] < minLvl) {
      minLvl = vol[i];
    } else if (vol[i] > maxLvl) {
      maxLvl = vol[i];
    }
  }

  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) {
    maxLvl = minLvl + TOP;
  }
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte wheelPos) {
  if (wheelPos < 85) {
   return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
  } else if (wheelPos < 170) {
   wheelPos -= 85;
   return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  } else {
   wheelPos -= 170;
   return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
}
