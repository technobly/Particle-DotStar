/*------------------------------------------------------------------------
  Particle library to control Adafruit DotStar addressable RGB LEDs.

  Ported by Technobly for Spark Core, Particle Photon, P1, Electron,
  RedBear Duo, Argon, Boron, Xenon, or Photon2/P2.

  ------------------------------------------------------------------------
  -- original header follows ---------------------------------------------
  ------------------------------------------------------------------------

/*!
 * @file Adafruit_DotStar.cpp
 *
 * @mainpage Arduino Library for driving Adafruit DotStar addressable LEDs
 * and compatible devicess -- APA102, etc.
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for Adafruit's DotStar library for the
 * Arduino platform, allowing a broad range of microcontroller boards
 * (most AVR boards, many ARM devices, ESP8266 and ESP32, among others)
 * to control Adafruit DotStars and compatible devices -- APA102, etc.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing products
 * from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried and Phil Burgess for Adafruit Industries with
 * contributions from members of the open source community.
 *
 * @section license License
 *
 * This file is part of the Adafruit_DotStar library.
 *
 * Adafruit_DotStar is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Adafruit_DotStar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with DotStar. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "dotstar.h"

#if PLATFORM_ID == 0 // Core (0)
  #define pinLO(_pin) (PIN_MAP[_pin].gpio_peripheral->BRR = PIN_MAP[_pin].gpio_pin)
  #define pinHI(_pin) (PIN_MAP[_pin].gpio_peripheral->BSRR = PIN_MAP[_pin].gpio_pin)
#elif (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
#if SYSTEM_VERSION < SYSTEM_VERSION_ALPHA(5,0,0,2)
  STM32_Pin_Info* PIN_MAP2 = HAL_Pin_Map(); // Pointer required for highest access speed
#else
  STM32_Pin_Info* PIN_MAP2 = hal_pin_map(); // Pointer required for highest access speed
#endif // SYSTEM_VERSION < SYSTEM_VERSION_ALPHA(5,0,0,2)
  #define pinLO(_pin) (PIN_MAP2[_pin].gpio_peripheral->BSRRH = PIN_MAP2[_pin].gpio_pin)
  #define pinHI(_pin) (PIN_MAP2[_pin].gpio_peripheral->BSRRL = PIN_MAP2[_pin].gpio_pin)
#elif HAL_PLATFORM_NRF52840 // Argon, Boron, Xenon, B SoM, B5 SoM, E SoM X, Tracker
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "pinmap_impl.h"
#if SYSTEM_VERSION < SYSTEM_VERSION_ALPHA(5,0,0,2)
  NRF5x_Pin_Info* PIN_MAP2 = HAL_Pin_Map();
#else
  NRF5x_Pin_Info* PIN_MAP2 = hal_pin_map();
#endif // SYSTEM_VERSION < SYSTEM_VERSION_ALPHA(5,0,0,2)
  #define pinLO(_pin) (nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(PIN_MAP2[_pin].gpio_port, PIN_MAP2[_pin].gpio_pin)))
  #define pinHI(_pin) (nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(PIN_MAP2[_pin].gpio_port, PIN_MAP2[_pin].gpio_pin)))
#elif (PLATFORM_ID == 32) // HAL_PLATFORM_RTL872X
  // nothing extra needed for P2
#else
  #error "*** PLATFORM_ID not supported by this library. PLATFORM should be Particle Core, Photon, Electron, Argon, Boron, Xenon, RedBear Duo, B SoM, B5 SoM, E SoM X, Tracker or P2 ***"
#endif
// fast pin access
#define pinSet(_pin, _hilo) (_hilo ? pinHI(_pin) : pinLO(_pin))

#if (PLATFORM_ID == 32)
void Adafruit_DotStar::spi_out(int n) {
    spi_->transfer(n);
}
#else
#define spi_out(n) (void)SPI.transfer(n)
#endif

#define USE_HW_SPI 255 // Assign this to dataPin to indicate 'hard' SPI

#if (PLATFORM_ID == 32)
Adafruit_DotStar::Adafruit_DotStar(uint16_t n, SPIClass& spi, uint8_t o) :
 numLEDs(n), dataPin(USE_HW_SPI), brightness(0), pixels(NULL),
 rOffset(o & 3), gOffset((o >> 2) & 3), bOffset((o >> 4) & 3)
{
  updateLength(n);
  spi_ = &spi;
}

#else
// Constructor for hardware SPI -- must connect to MOSI, SCK pins
Adafruit_DotStar::Adafruit_DotStar(uint16_t n, uint8_t o) :
 numLEDs(n), dataPin(USE_HW_SPI), brightness(0), pixels(NULL),
 rOffset(o & 3), gOffset((o >> 2) & 3), bOffset((o >> 4) & 3)
{
  updateLength(n);
}

// Constructor for 'soft' (bitbang) SPI -- any two pins can be used
Adafruit_DotStar::Adafruit_DotStar(uint16_t n, uint8_t data, uint8_t clock,
  uint8_t o) :
 dataPin(data), clockPin(clock), brightness(0), pixels(NULL),
 rOffset(o & 3), gOffset((o >> 2) & 3), bOffset((o >> 4) & 3)
{
  updateLength(n);
}
#endif // #if (PLATFORM_ID == 32)

Adafruit_DotStar::~Adafruit_DotStar(void) { // Destructor
  if (pixels)                free(pixels);
  if (dataPin == USE_HW_SPI) hw_spi_end();
  else                       sw_spi_end();
}

void Adafruit_DotStar::begin(void) { // Initialize SPI
  if (dataPin == USE_HW_SPI) hw_spi_init();
  else                       sw_spi_init();
}

// Pins may be reassigned post-begin(), so a sketch can store hardware
// config in flash, SD card, etc. rather than hardcoded.  Also permits
// "recycling" LED ram across multiple strips: set pins to first strip,
// render & write all data, reassign pins to next strip, render & write,
// etc.  They won't update simultaneously, but usually unnoticeable.

// Change to hardware SPI -- must connect to MOSI, SCK pins
void Adafruit_DotStar::updatePins(void) {
#if PLATFORM_ID != 32
  sw_spi_end();
  dataPin = USE_HW_SPI;
  hw_spi_init();
#endif
}

// Change to 'soft' (bitbang) SPI -- any two pins can be used
void Adafruit_DotStar::updatePins(uint8_t data, uint8_t clock) {
#if PLATFORM_ID != 32
  hw_spi_end();
  dataPin  = data;
  clockPin = clock;
  sw_spi_init();
#endif
}

// Length can be changed post-constructor for similar reasons (sketch
// config not hardcoded).  But DON'T use this for "recycling" strip RAM...
// all that reallocation is likely to fragment and eventually fail.
// Instead, set length once to longest strip.
void Adafruit_DotStar::updateLength(uint16_t n) {
  if (pixels) free(pixels);
  uint16_t bytes = n * 3;
  if ((pixels = (uint8_t *)malloc(bytes))) {
    numLEDs = n;
    clear();
  } else {
    numLEDs = 0;
  }
}

// SPI STUFF ---------------------------------------------------------------

void Adafruit_DotStar::hw_spi_init(void) { // Initialize hardware SPI
#if (PLATFORM_ID != 32)
  SPI.begin();
  // 72MHz / 4 = 18MHz (sweet spot)
  // Any slower than 18MHz and you are barely faster than Software SPI.
  // Any faster than 18MHz and the code overhead dominates.
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
#else
  spi_->begin(PIN_INVALID);
  spi_->setClockSpeed(12500000);
  spi_->setBitOrder(MSBFIRST);
  spi_->setDataMode(SPI_MODE0);
#endif
}

void Adafruit_DotStar::hw_spi_end(void) { // Stop hardware SPI
#if (PLATFORM_ID != 32)
  SPI.end();
#else
  spi_->end();
#endif
}

void Adafruit_DotStar::sw_spi_init(void) { // Init 'soft' (bitbang) SPI
#if (PLATFORM_ID != 32)
  pinMode(dataPin , OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinSet(dataPin , LOW);
  pinSet(clockPin, LOW);
#endif
}

void Adafruit_DotStar::sw_spi_end() { // Stop 'soft' SPI
#if (PLATFORM_ID != 32)
  pinMode(dataPin , INPUT);
  pinMode(clockPin, INPUT);
#endif
}

void Adafruit_DotStar::sw_spi_out(uint8_t n) { // Bitbang SPI write
#if (PLATFORM_ID != 32)
  for (uint8_t i=8; i--; n <<= 1) {
    if (n & 0x80) pinSet(dataPin, HIGH);
    else          pinSet(dataPin, LOW);
    pinSet(clockPin, HIGH);
    pinSet(clockPin, LOW);
  }
#endif
}

/* ISSUE DATA TO LED STRIP -------------------------------------------------

  Although the LED driver has an additional per-pixel 5-bit brightness
  setting, it is NOT used or supported here because it's a brain-dead
  misfeature that's counter to the whole point of Dot Stars, which is to
  have a much faster PWM rate than NeoPixels.  It gates the high-speed
  PWM output through a second, much slower PWM (about 400 Hz), rendering
  it useless for POV.  This brings NOTHING to the table that can't be
  already handled better in one's sketch code.  If you really can't live
  without this abomination, you can fork the library and add it for your
  own use, but any pull requests for this will NOT be merged, nuh uh!
*/

void Adafruit_DotStar::show(void) {

  if (!pixels) return;

  uint8_t *ptr = pixels, i;            // -> LED data
  uint16_t n   = numLEDs;              // Counter
  uint16_t b16 = (uint16_t)brightness; // Type-convert for fixed-point math

  //__disable_irq(); // If 100% focus on SPI clocking required

  if (dataPin == USE_HW_SPI) {

    // [START FRAME]
    for (i = 0; i < 4; i++) {
      spi_out(0);                        // Start-frame marker
    }
    // [PIXEL DATA]
    if (brightness) {                    // Scale pixel brightness on output
      do {                               // For each pixel...
        spi_out(0xFF);                   //  Pixel start
        for (i = 0; i < 3; i++) {
          spi_out((*ptr++ * b16) >> 8);  // Scale, write RGB
        }
      } while (--n);
    } else {                             // Full brightness (no scaling)
      do {                               // For each pixel...
        spi_out(0xFF);                   //  Pixel start
        for (i = 0; i < 3; i++) {
          spi_out(*ptr++);               // Write R,G,B
        }
      } while (--n);
    }
    // [END FRAME]
    // Four end-frame bytes are seemingly indistinguishable from a white
    // pixel, and empirical testing suggests it can be left out...but it's
    // always a good idea to follow the datasheet, in case future hardware
    // revisions are more strict (e.g. might mandate use of end-frame
    // before start-frame marker). i.e. let's not remove this. But after
    // testing a bit more the suggestion is to use at least (numLeds+1)/2
    // high values (1) or (numLeds+15)/16 full bytes as EndFrame. For details
    // see also:
    // https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
    for (i = 0; i < ((numLEDs + 15) / 16); i++) {
      spi_out(0xFF);
    }

  } else {                               // Soft (bitbang) SPI

    // [START FRAME]
    for (i = 0; i < 4; i++) {
      sw_spi_out(0);                     // Start-frame marker
    }
    // [PIXEL DATA]
    if (brightness) {                    // Scale pixel brightness on output
      do {                               // For each pixel...
        sw_spi_out(0xFF);                //  Pixel start
        for (i = 0; i < 3; i++) {
          sw_spi_out((*ptr++ * b16) >> 8); // Scale, write
        }
      } while (--n);
    } else {                             // Full brightness (no scaling)
      do {                               // For each pixel...
        sw_spi_out(0xFF);                //  Pixel start
        for (i = 0; i < 3; i++) {
          sw_spi_out(*ptr++);            // R,G,B
        }
      } while (--n);
    }
    // [END FRAME]
    // Four end-frame bytes are seemingly indistinguishable from a white
    // pixel, and empirical testing suggests it can be left out...but it's
    // always a good idea to follow the datasheet, in case future hardware
    // revisions are more strict (e.g. might mandate use of end-frame
    // before start-frame marker). i.e. let's not remove this. But after
    // testing a bit more the suggestion is to use at least (numLeds+1)/2
    // high values (1) or (numLeds+15)/16 full bytes as EndFrame. For details
    // see also:
    // https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
    for (i = 0; i < ((numLEDs + 15) / 16); i++) {
      sw_spi_out(0xFF);
    }
  }

  //__enable_irq();
}

void Adafruit_DotStar::clear() { // Write 0s (off) to full pixel buffer
  memset(pixels, 0, numLEDs * 3);
}

// Set pixel color, separate R,G,B values (0-255 ea.)
void Adafruit_DotStar::setPixelColor(
 uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n < numLEDs) {
    uint8_t *p = &pixels[n * 3];
    p[rOffset] = r;
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

// Set pixel color, 'packed' RGB value (0x000000 - 0xFFFFFF)
void Adafruit_DotStar::setPixelColor(uint16_t n, uint32_t c) {
  if (n < numLEDs) {
    uint8_t *p = &pixels[n * 3];
    p[rOffset] = (uint8_t)(c >> 16);
    p[gOffset] = (uint8_t)(c >>  8);
    p[bOffset] = (uint8_t)c;
  }
}

// Read color from previously-set pixel, returns packed RGB value.
uint32_t Adafruit_DotStar::getPixelColor(uint16_t n) const {
  if (n >= numLEDs) return 0;
  uint8_t *p = &pixels[n * 3];
  return ((uint32_t)p[rOffset] << 16) |
         ((uint32_t)p[gOffset] <<  8) |
          (uint32_t)p[bOffset];
}

uint16_t Adafruit_DotStar::numPixels(void) { // Ret. strip length
  return numLEDs;
}

// Set global strip brightness.  This does not have an immediate effect;
// must be followed by a call to show().  Not a fan of this...for various
// reasons I think it's better handled in one's sketch, but it's here for
// parity with the NeoPixel library.  Good news is that brightness setting
// in this library is 'non destructive' -- it's applied as color data is
// being issued to the strip, not during setPixel(), and also means that
// getPixelColor() returns the exact value originally stored.
void Adafruit_DotStar::setBrightness(uint8_t b) {
  // Stored brightness value is different than what's passed.  This
  // optimizes the actual scaling math later, allowing a fast 8x8-bit
  // multiply and taking the MSB.  'brightness' is a uint8_t, adding 1
  // here may (intentionally) roll over...so 0 = max brightness (color
  // values are interpreted literally; no scaling), 1 = min brightness
  // (off), 255 = just below max brightness.
  brightness = b + 1;
}

uint8_t Adafruit_DotStar::getBrightness(void) const {
  return brightness - 1; // Reverse above operation
}

// Return pointer to the library's pixel data buffer.  Use carefully,
// much opportunity for mayhem.  It's mostly for code that needs fast
// transfers, e.g. SD card to LEDs.  Color data is in BGR order.
uint8_t *Adafruit_DotStar::getPixels(void) const {
  return pixels;
}

/*!
  @brief   Fill all or part of the DotStar strip with a color.
  @param   c      32-bit color value. Most significant byte is 0, second
                  is red, then green, and least significant byte is blue.
                  e.g. 0x00RRGGBB. If all arguments are unspecified, this
                  will be 0 (off).
  @param   first  Index of first pixel to fill, starting from 0. Must be
                  in-bounds, no clipping is performed. 0 if unspecified.
  @param   count  Number of pixels to fill, as a positive value. Passing
                  0 or leaving unspecified will fill to end of strip.
*/
void Adafruit_DotStar::fill(uint32_t c, uint16_t first, uint16_t count) {
  uint16_t i, end;

  if (first >= numLEDs) {
    return; // If first LED is past end of strip, nothing to do
  }

  // Calculate the index ONE AFTER the last pixel to fill
  if (count == 0) {
    // Fill to end of strip
    end = numLEDs;
  } else {
    // Ensure that the loop won't go past the last pixel
    end = first + count;
    if (end > numLEDs)
      end = numLEDs;
  }

  for (i = first; i < end; i++) {
    this->setPixelColor(i, c);
  }
}

/*!
  @brief   Convert hue, saturation and value into a packed 32-bit RGB color
           that can be passed to setPixelColor() or other RGB-compatible
           functions.
  @param   hue  An unsigned 16-bit value, 0 to 65535, representing one full
                loop of the color wheel, which allows 16-bit hues to "roll
                over" while still doing the expected thing (and allowing
                more precision than the wheel() function that was common to
                prior DotStar and NeoPixel examples).
  @param   sat  Saturation, 8-bit value, 0 (min or pure grayscale) to 255
                (max or pure hue). Default of 255 if unspecified.
  @param   val  Value (brightness), 8-bit value, 0 (min / black / off) to
                255 (max or full brightness). Default of 255 if unspecified.
  @return  Packed 32-bit RGB color. Result is linearly but not perceptually
           correct, so you may want to pass the result through the gamma32()
           function (or your own gamma-correction operation) else colors may
           appear washed out. This is not done automatically by this
           function because coders may desire a more refined gamma-
           correction function than the simplified one-size-fits-all
           operation of gamma32(). Diffusing the LEDs also really seems to
           help when using low-saturation colors.
*/
uint32_t Adafruit_DotStar::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {

  uint8_t r, g, b;

  // Remap 0-65535 to 0-1529. Pure red is CENTERED on the 64K rollover;
  // 0 is not the start of pure red, but the midpoint...a few values above
  // zero and a few below 65536 all yield pure red (similarly, 32768 is the
  // midpoint, not start, of pure cyan). The 8-bit RGB hexcone (256 values
  // each for red, green, blue) really only allows for 1530 distinct hues
  // (not 1536, more on that below), but the full unsigned 16-bit type was
  // chosen for hue so that one's code can easily handle a contiguous color
  // wheel by allowing hue to roll over in either direction.
  hue = (hue * 1530L + 32768) / 65536;
  // Because red is centered on the rollover point (the +32768 above,
  // essentially a fixed-point +0.5), the above actually yields 0 to 1530,
  // where 0 and 1530 would yield the same thing. Rather than apply a
  // costly modulo operator, 1530 is handled as a special case below.

  // So you'd think that the color "hexcone" (the thing that ramps from
  // pure red, to pure yellow, to pure green and so forth back to red,
  // yielding six slices), and with each color component having 256
  // possible values (0-255), might have 1536 possible items (6*256),
  // but in reality there's 1530. This is because the last element in
  // each 256-element slice is equal to the first element of the next
  // slice, and keeping those in there this would create small
  // discontinuities in the color wheel. So the last element of each
  // slice is dropped...we regard only elements 0-254, with item 255
  // being picked up as element 0 of the next slice. Like this:
  // Red to not-quite-pure-yellow is:        255,   0, 0 to 255, 254,   0
  // Pure yellow to not-quite-pure-green is: 255, 255, 0 to   1, 255,   0
  // Pure green to not-quite-pure-cyan is:     0, 255, 0 to   0, 255, 254
  // and so forth. Hence, 1530 distinct hues (0 to 1529), and hence why
  // the constants below are not the multiples of 256 you might expect.

  // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
  if (hue < 510) { // Red to Green-1
    b = 0;
    if (hue < 255) { //   Red to Yellow-1
      r = 255;
      g = hue;       //     g = 0 to 254
    } else {         //   Yellow to Green-1
      r = 510 - hue; //     r = 255 to 1
      g = 255;
    }
  } else if (hue < 1020) { // Green to Blue-1
    r = 0;
    if (hue < 765) { //   Green to Cyan-1
      g = 255;
      b = hue - 510;  //     b = 0 to 254
    } else {          //   Cyan to Blue-1
      g = 1020 - hue; //     g = 255 to 1
      b = 255;
    }
  } else if (hue < 1530) { // Blue to Red-1
    g = 0;
    if (hue < 1275) { //   Blue to Magenta-1
      r = hue - 1020; //     r = 0 to 254
      b = 255;
    } else { //   Magenta to Red-1
      r = 255;
      b = 1530 - hue; //     b = 255 to 1
    }
  } else { // Last 0.5 Red (quicker than % operator)
    r = 255;
    g = b = 0;
  }

  // Apply saturation and value to R,G,B, pack into 32-bit result:
  uint32_t v1 = 1 + val;  // 1 to 256; allows >>8 instead of /255
  uint16_t s1 = 1 + sat;  // 1 to 256; same reason
  uint8_t s2 = 255 - sat; // 255 to 0
  return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
         (((((g * s1) >> 8) + s2) * v1) & 0xff00) |
         (((((b * s1) >> 8) + s2) * v1) >> 8);
}

/*!
  @brief   A gamma-correction function for 32-bit packed RGB colors.
           Makes color transitions appear more perceptially correct.
  @param   x  32-bit packed RGB color.
  @return  Gamma-adjusted packed color, can then be passed in one of the
           setPixelColor() functions. Like gamma8(), this uses a fixed
           gamma correction exponent of 2.6, which seems reasonably okay
           for average DotStars in average tasks. If you need finer
           control you'll need to provide your own gamma-correction
           function instead.
*/
uint32_t Adafruit_DotStar::gamma32(uint32_t x) {
  uint8_t *y = (uint8_t *)&x;
  // All four bytes of a 32-bit value are filtered to avoid a bunch of
  // shifting and masking that would be necessary for properly handling
  // different endianisms (and each byte is a fairly trivial operation,
  // so it might not even be wasting cycles vs a check and branch.
  // In theory this might cause trouble *if* someone's storing information
  // in the unused most significant byte of an RGB value, but this seems
  // exceedingly rare and if it's encountered in reality they can mask
  // values going in or coming out.
  for (uint8_t i = 0; i < 4; i++)
    y[i] = gamma8(y[i]);
  return x; // Packed 32-bit return
}

/*!
  @brief   Fill DotStar strip with one or more cycles of hues.
           Everyone loves the rainbow swirl so much, now it's canon!
  @param   first_hue   Hue of first pixel, 0-65535, representing one full
                       cycle of the color wheel. Each subsequent pixel will
                       be offset to complete one or more cycles over the
                       length of the strip.
  @param   reps        Number of cycles of the color wheel over the length
                       of the strip. Default is 1. Negative values can be
                       used to reverse the hue order.
  @param   saturation  Saturation (optional), 0-255 = gray to pure hue,
                       default = 255.
  @param   brightness  Brightness/value (optional), 0-255 = off to max,
                       default = 255. This is distinct and in combination
                       with any configured global strip brightness.
  @param   gammify     If true (default), apply gamma correction to colors
                       for better appearance.
*/
void Adafruit_DotStar::rainbow(uint16_t first_hue, int8_t reps,
                               uint8_t saturation, uint8_t brightness,
                               bool gammify) {
  for (uint16_t i = 0; i < numLEDs; i++) {
    uint16_t hue = first_hue + (i * reps * 65536) / numLEDs;
    uint32_t color = ColorHSV(hue, saturation, brightness);
    if (gammify)
      color = gamma32(color);
    setPixelColor(i, color);
  }
}
