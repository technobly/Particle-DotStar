/*------------------------------------------------------------------------
  Particle library to control Adafruit DotStar addressable RGB LEDs.

  Ported by Technobly for Spark Core, Particle Photon, P1, Electron,
  RedBear Duo, Argon, Boron, Xenon, or Photon2/P2.

  ------------------------------------------------------------------------
  -- original header follows ---------------------------------------------
  ------------------------------------------------------------------------

 * Simple strand test for Adafruit Dot Star RGB LED strip.
 * This is a basic diagnostic tool, NOT a graphics demo...helps confirm
 * correct wiring and tests each pixel's ability to display red, green
 * and blue and to forward data down the line.  By limiting the number
 * and color of LEDs, it's reasonably safe to power a couple meters off
 * the VIN pin.  DON'T try that with other code!
 */

/* ======================= includes ================================= */

#include "Particle.h"

#include "dotstar.h"

#define NUMPIXELS 30 // Number of LEDs in strip

//-------------------------------------------------------------------
// NOTE: If you find that the colors you choose are not correct,
// there is an optional 2nd argument (for HW SPI) and
// 4th arg. (for SW SPI) that you may specify to correct the colors.
//-------------------------------------------------------------------
// e.g. Adafruit_DotStar(NUMPIXELS, DOTSTAR_BGR);
// e.g. Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
//
// DOTSTAR_RGB
// DOTSTAR_RBG
// DOTSTAR_GRB
// DOTSTAR_GBR
// DOTSTAR_BRG
// DOTSTAR_BGR (default)

#if (PLATFORM_ID == 32) // P2/Photon2
//-------------------------------------------------------------------
// P2/Photon2 must use dedicated SPI Interface API (Hardware SPI/SPI1):
// e.g. Adafruit_DotStar(NUMPIXELS, SPI_INTERFACE, DOTSTAR_BGR);
//-------------------------------------------------------------------
// SPI: MO (data), SCK (clock)
#define SPI_INTERFACE SPI
// SPI1: D2 (data), D4 (clock)
// #define SPI_INTERFACE SPI1
Adafruit_DotStar strip(NUMPIXELS, SPI_INTERFACE, DOTSTAR_BGR);

#else // Argon, Boron, etc..
//-------------------------------------------------------------------
// Here's how to control the LEDs from any two pins (Software SPI):
// e.g. Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
//-------------------------------------------------------------------
#define DATAPIN   MOSI
#define CLOCKPIN  SCK
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

//-------------------------------------------------------------------
// Here's how to control the LEDs from SPI pins (Hardware SPI):
// e.g. Adafruit_DotStar(NUMPIXELS, DOTSTAR_RGB);
//-------------------------------------------------------------------
// Hardware SPI is a little faster, but must be wired to specific pins
// (Core/Photon/P1/Electron = pin A5 for data, A3 for clock)
//Adafruit_DotStar strip(NUMPIXELS, DOTSTAR_BGR);

#endif // #if (PLATFORM_ID == 32)

void setup() {
  strip.begin(); // Initialize pins for output
  strip.setBrightness(10); // 0-255
  strip.show();  // Update all LEDs, turning them off
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

  if (++head >= NUMPIXELS) {        // Increment head index.  Off end of strip?
    head = 0;                       //  Yes, reset head index to start
    if ((color >>= 8) == 0) {       //  Next color (R->G->B) ... past blue now?
      color = 0xFF0000;             //   Yes, reset to red
    }
  }
  if (++tail >= NUMPIXELS) {
    tail = 0;                       // Increment, reset tail index
  }
}
