/*------------------------------------------------------------------------
  Spark Core library to control Adafruit DotStar addressable RGB LEDs.

  Ported to Spark Core by Technobly.

  ------------------------------------------------------------------------
  -- original header follows ---------------------------------------------
  ------------------------------------------------------------------------

// Simple strand test for Adafruit Dot Star RGB LED strip.
// This is a basic diagnostic tool, NOT a graphics demo...helps confirm
// correct wiring and tests each pixel's ability to display red, green
// and blue and to forward data down the line.  By limiting the number
// and color of LEDs, it's reasonably safe to power a couple meters off
// the Spark Core's VIN pin.  DON'T try that with other code!

/* ======================= includes ================================= */

#include "application.h"

#include "dotstar/dotstar.h"

#define NUMPIXELS 30 // Number of LEDs in strip

//-------------------------------------------------------------------
// NOTE: If you find that the colors you choose are not correct,
// there is an optional 2nd argument (for HW SPI) and 
// 4th arg. (for SW SPI) that you may specify to correct the colors.
//-------------------------------------------------------------------
// e.g. Adafruit_DotStar(NUMPIXELS, DOTSTAR_RGB);
// e.g. Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_RGB);
//
// DOTSTAR_RGB
// DOTSTAR_RBG
// DOTSTAR_GRB
// DOTSTAR_GBR (default)
// DOTSTAR_BRG
// DOTSTAR_BGR 

//-------------------------------------------------------------------
// Here's how to control the LEDs from any two pins (Software SPI):
//-------------------------------------------------------------------
#define DATAPIN   D4
#define CLOCKPIN  D5
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN);

//-------------------------------------------------------------------
// Here's how to control the LEDs from any two pins (Hardware SPI):
//-------------------------------------------------------------------
// Hardware SPI is a little faster, but must be wired to specific pins
// (Spark Core = pin A5 for data, A3 for clock)
//Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS);

void setup() {
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}

// Runs 10 LEDs at a time along strip, cycling through red, green and blue.
// This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.

int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)

void loop() {

  strip.setPixelColor(head, color); // 'On' pixel at head
  strip.setPixelColor(tail, 0);     // 'Off' pixel at tail
  strip.show();                     // Refresh strip
  delay(20);                        // Pause 20 milliseconds (~50 FPS)

  if(++head >= NUMPIXELS) {         // Increment head index.  Off end of strip?
    head = 0;                       //  Yes, reset head index to start
    if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
      color = 0xFF0000;             //   Yes, reset to red
  }
  if(++tail >= NUMPIXELS) tail = 0; // Increment, reset tail index
}
