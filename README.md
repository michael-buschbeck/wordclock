This is the development repository for the **Word Clock**,
powered by Arduino, a cheap DCF77 receiver module, and almost two meters of WS2812B LED strip.

Uses the brilliant [FastLED](http://fastled.io) library to control the LEDs
and my own [robust DCF77 library](https://github.com/michael-buschbeck/arduino/tree/master/DCF77)
to make sense of the bit salad produced by the cheap Pollin DCF77 receiver module,
especially when trying to use it somewhere close to the middle of England.

====

(C) 2015 Michael Buschbeck (michael@buschbeck.net)

Licensed under a Creative Commons Attribution 4.0 International License
and distributed in the hope that it will be useful, but without any warranty.
See http://creativecommons.org/licenses/by/4.0/ for details.
