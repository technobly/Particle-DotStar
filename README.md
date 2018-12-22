Particle-DotStar
================

A library for manipulating DotStar RGB LEDs for the Spark Core, Particle Photon, P1, Electron, and RedBear Duo.

Implementation based on Adafruit's DotStar Library.

DotStar LED's are APA102: [Datasheet](http://www.adafruit.com/datasheets/APA102.pdf)

Components Required
---
- A DotStar digital RGB LED (get at [adafruit.com](http://www.adafruit.com/search?q=dotstar&b=1))
- A Particle Shield Shield or breakout board to supply DotStars with 5V (see store at [particle.io](particle.io))

Example Usage
---

```cpp
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN);
void setup() {
  strip.begin();
  strip.show();
}
void loop() {
  // change your pixel colors and call strip.show() again
}
```

Nuances
---

- Make sure get the # of pixels, clock and data pin numbers (SW SPI can be any pins, HW SPI only uses the pins defined for the SPI peripheral on your device)

- DotStars require 5V logic level inputs and Particle devices only have 3.3V logic level digital outputs. You may find level shifting from 3.3V to 5V necessary if your LED strips are not updating properly. The Spark Shield Shield has the [TXB0108PWR](http://www.digikey.com/product-search/en?pv7=2&k=TXB0108PWR) 3.3V to 5V level shifter built in (but has been known to oscillate at 50MHz with wire length longer than 6"), alternatively you can wire up your own with a [SN74HCT245N](http://www.digikey.com/product-detail/en/SN74HCT245N/296-1612-5-ND/277258), or [SN74HCT125N](http://www.digikey.com/product-detail/en/SN74HCT125N/296-8386-5-ND/376860). These are rock solid.


Useful Links
---

- DotStar Guide: https://learn.adafruit.com/adafruit-dotstar-leds
- Quad Level Shifter IC: [SN74ACHT125N](https://www.adafruit.com/product/1787) (Adafruit)
- Quad Level Shifter IC: [SN74HCT125N](http://www.digikey.com/product-detail/en/SN74HCT125N/296-8386-5-ND/376860) (Digikey)
- Quad Level Shifter IC: [SN74AHCT125N](http://www.digikey.com/product-detail/en/SN74AHCT125N/296-4655-5-ND/375798) (Digikey)
