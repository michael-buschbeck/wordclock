///////////////////////////////////////////////////////////////////////////////
//
//  Word Clock powered by Arduino, a DCF77 receiver and some WS2812B LEDs.
//
//  (C) 2015 Michael Buschbeck <michael@buschbeck.net>
//
//  Licensed under a Creative Commons Attribution 4.0 International License.
//  Distributed in the hope that it will be useful, but without any warranty.
//
//  See <http://creativecommons.org/licenses/by/4.0/> for details.
//


#include <FastLED.h>
#include <TimeLib.h>

#include <DCF77.h>
#include <Timer.h>
#include "WordClock.h"


// iCol =       0  1  2  3  4  5  6  7  8  9 10
//              |  |  |  |  |  |  |  |  |  |  |
// iRow =  0    E  S  .  I  S  T  .  F  Ü  N  F
// iRow =  1    Z  E  H  N  V  I  E  R  T  E  L
// iRow =  2    .  V  O  R  .  N  A  C  H  .  .
// iRow =  3    .  H  A  P  P  Y  .  H  A  L  B
// iRow =  4    Z  W  E  I  .  N  E  W  .  .  .
// iRow =  5    B  I  R  T  H  D  A  Y  E  A  R
// iRow =  6    D  R  E  I  N  S  E  C  H  S  .
// iRow =  7    S  I  E  B  E  N  .  V  I  V  I
// iRow =  8    A  C  H  T  Z  E  H  N  E  U  N
// iRow =  9    Z  W  Ö  L  F  Ü  N  F  E  L  F
// iRow = 10    M  J  .  V  I  E  R  .  U  H  R
//              |  |  |  |  |  |  |  |  |  |  |
// iCol =       0  1  2  3  4  5  6  7  8  9 10

constexpr Word WORD_STATIC_ES       (0,  0,  2);
constexpr Word WORD_STATIC_IST      (0,  3,  3);
constexpr Word WORD_STATIC_UHR     (10,  8,  3);
      
constexpr Word WORD_MIN_FUENF       (0,  7,  4);
constexpr Word WORD_MIN_ZEHN        (1,  0,  4);
constexpr Word WORD_MIN_VIERTEL     (1,  4,  7);
constexpr Word WORD_MIN_VOR         (2,  1,  3);
constexpr Word WORD_MIN_NACH        (2,  5,  4);
constexpr Word WORD_MIN_HALB        (3,  7,  4);
      
constexpr Word WORD_HOUR_EIN        (6,  2,  3);
constexpr Word WORD_HOUR_EINS       (6,  2,  4);
constexpr Word WORD_HOUR_ZWEI       (4,  0,  4);
constexpr Word WORD_HOUR_DREI       (6,  0,  4);
constexpr Word WORD_HOUR_VIER      (10,  3,  4);
constexpr Word WORD_HOUR_FUENF      (9,  4,  4);
constexpr Word WORD_HOUR_SECHS      (6,  5,  5);
constexpr Word WORD_HOUR_SIEBEN     (7,  0,  6);
constexpr Word WORD_HOUR_ACHT       (8,  0,  4);
constexpr Word WORD_HOUR_NEUN       (8,  7,  4);
constexpr Word WORD_HOUR_ZEHN       (8,  4,  4);
constexpr Word WORD_HOUR_ELF        (9,  8,  3);
constexpr Word WORD_HOUR_ZWOELF     (9,  0,  5);
      
constexpr Word WORD_EXTRA_HAPPY     (3,  1,  5);
constexpr Word WORD_EXTRA_BIRTHDAY  (5,  0,  8);
constexpr Word WORD_EXTRA_VIVI      (7,  7,  4);
constexpr Word WORD_EXTRA_MJ       (10,  0,  2);
constexpr Word WORD_EXTRA_NEW       (4,  5,  3);
constexpr Word WORD_EXTRA_YEAR      (5,  7,  4);


enum Extra
{
  EXTRA_NONE,
  EXTRA_BIRTHDAY,
  EXTRA_NEWYEAR,
};


void renderExtra(Layer& layer, Extra const extra)
{
  switch (extra) {
    case EXTRA_NONE:
      break;

    case EXTRA_BIRTHDAY:
      WORD_EXTRA_HAPPY    .render(layer);
      WORD_EXTRA_BIRTHDAY .render(layer);
      WORD_EXTRA_VIVI     .render(layer);
      WORD_EXTRA_MJ       .render(layer);
      break;

    case EXTRA_NEWYEAR:
      WORD_EXTRA_HAPPY    .render(layer);
      WORD_EXTRA_NEW      .render(layer);
      WORD_EXTRA_YEAR     .render(layer);
      break;
  }
}


void renderTime(Layer& layer, uint8_t const hour, uint8_t const minute)
{
  uint8_t       indexHour   = hour;
  uint8_t const indexMinute = minute / 5;

  WORD_STATIC_ES  .render(layer);
  WORD_STATIC_IST .render(layer);

  switch (indexMinute) {
    case  0:                                                                                                                break;
    case  1:  WORD_MIN_FUENF   .render(layer);  WORD_MIN_NACH .render(layer);                                               break;
    case  2:  WORD_MIN_ZEHN    .render(layer);  WORD_MIN_NACH .render(layer);                                               break;
    case  3:  WORD_MIN_VIERTEL .render(layer);  WORD_MIN_NACH .render(layer);                                               break;
    case  4:  WORD_MIN_ZEHN    .render(layer);  WORD_MIN_VOR  .render(layer);  WORD_MIN_HALB .render(layer);  ++indexHour;  break;
    case  5:  WORD_MIN_FUENF   .render(layer);  WORD_MIN_VOR  .render(layer);  WORD_MIN_HALB .render(layer);  ++indexHour;  break;
    case  6:                                                                   WORD_MIN_HALB .render(layer);  ++indexHour;  break;
    case  7:  WORD_MIN_FUENF   .render(layer);  WORD_MIN_NACH .render(layer);  WORD_MIN_HALB .render(layer);  ++indexHour;  break;
    case  8:  WORD_MIN_ZEHN    .render(layer);  WORD_MIN_NACH .render(layer);  WORD_MIN_HALB .render(layer);  ++indexHour;  break;
    case  9:  WORD_MIN_VIERTEL .render(layer);  WORD_MIN_VOR  .render(layer);                                 ++indexHour;  break;
    case 10:  WORD_MIN_ZEHN    .render(layer);  WORD_MIN_VOR  .render(layer);                                 ++indexHour;  break;
    case 11:  WORD_MIN_FUENF   .render(layer);  WORD_MIN_VOR  .render(layer);                                 ++indexHour;  break;
  }

  switch (indexHour % 12) {
    case  0:                        WORD_HOUR_ZWOELF .render(layer);  break;
    case  1:  if (indexMinute == 0) WORD_HOUR_EIN    .render(layer);
              else                  WORD_HOUR_EINS   .render(layer);  break;
    case  2:                        WORD_HOUR_ZWEI   .render(layer);  break;
    case  3:                        WORD_HOUR_DREI   .render(layer);  break;
    case  4:                        WORD_HOUR_VIER   .render(layer);  break;
    case  5:                        WORD_HOUR_FUENF  .render(layer);  break;
    case  6:                        WORD_HOUR_SECHS  .render(layer);  break;
    case  7:                        WORD_HOUR_SIEBEN .render(layer);  break;
    case  8:                        WORD_HOUR_ACHT   .render(layer);  break;
    case  9:                        WORD_HOUR_NEUN   .render(layer);  break;
    case 10:                        WORD_HOUR_ZEHN   .render(layer);  break;
    case 11:                        WORD_HOUR_ELF    .render(layer);  break;
  }

  if (indexMinute == 0)
    WORD_STATIC_UHR .render(layer);
}


void renderCountdown(Layer& layer, uint8_t const seconds)
{
  switch (seconds) {
    case 10:  WORD_MIN_ZEHN    .render(layer);  break;
    case  9:  WORD_HOUR_NEUN   .render(layer);  break;
    case  8:  WORD_HOUR_ACHT   .render(layer);  break;
    case  7:  WORD_HOUR_SIEBEN .render(layer);  break;
    case  6:  WORD_HOUR_SECHS  .render(layer);  break;
    case  5:  WORD_HOUR_FUENF  .render(layer);  break;
    case  4:  WORD_HOUR_VIER   .render(layer);  break;
    case  3:  WORD_HOUR_DREI   .render(layer);  break;
    case  2:  WORD_HOUR_ZWEI   .render(layer);  break;
    case  1:  WORD_HOUR_EINS   .render(layer);  break;
  }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Pins
//

constexpr uint8_t pinLed          = A0;  // OUTPUT
constexpr uint8_t pinReceiverData = A1;  // INPUT
constexpr uint8_t pinReceiverOff  = A2;  // OUTPUT


///////////////////////////////////////////////////////////////////////////////
//
//  DCF77 Receiver
//

enum DCF77State
{
  DCF77_STATE_PENDING,
  DCF77_STATE_RECEIVED,
  DCF77_STATE_IGNORED
};


DCF77Timestamp timestamp;
bool processTimestamp = false;

DCF77State stateReceiver = DCF77_STATE_PENDING;


void receivedTimestamp(DCF77Timestamp const & timestampReceived)
{
  processTimestamp = true;
  timestamp = timestampReceived;
}


DCF77Receiver receiver (
  receivedTimestamp
);


// Used only for debug display
uint8_t samplesReceived[32 * 2 / 8];

DCF77Receiver::Bitptr bitptrSamplesReceived;
DCF77Receiver::Bitptr bitptrSamplesDisplayed;


void setupReceiver()
{
  cli();
  
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Set compare match register:
  // clock speed (16_000_000) / interrupts per second (32) / prescaler (8) - 1
  OCR1A = 62499;

  // Set clear timer on compare match (CTC) mode
  TCCR1B |= (1 << WGM12);

  // Set prescaler to 8
  TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);

  // Enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  sei();

  // Clear debug display sample buffer
  memset(samplesReceived, 0xFF, sizeof(samplesReceived));
}


ISR(TIMER1_COMPA_vect)
{
  bool const sample = (digitalRead(pinReceiverData) == 0 ? true : false);

  bitptrSamplesReceived.set(samplesReceived, sample);

  ++bitptrSamplesReceived;
  if (bitptrSamplesReceived == sizeof(samplesReceived) * 8)
    bitptrSamplesReceived = 0;
  
  receiver.processSample(sample, millis());
}


void printSamples()
{
  Serial.print(F("DCF77: "));

  for (uint8_t iSample = 0; iSample < 32; ++iSample) {
    bool const sample = bitptrSamplesDisplayed.get(samplesReceived);

    ++bitptrSamplesDisplayed;
    if (bitptrSamplesDisplayed == sizeof(samplesReceived) * 8)
      bitptrSamplesDisplayed = 0;

    Serial.print(sample ? 'X' : '-');
  }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Display
//

CRGB leds[Layout::size()];

TransitionWave transitionWave
(
  CHSV (0x20, 0x80, 0x60),
  [] (uint8_t const iRow, uint8_t const iCol) -> uint16_t { return sqrt(50.0*50.0 * (pow(iRow - 5.0, 2.0) + pow(iCol - 5.0, 2.0))); },
  [] (uint8_t const iRow, uint8_t const iCol) -> uint16_t { return 1000; }
);

TransitionBurn transitionBurn
(
  CHSV (0x20, 0x80, 0xFF),
  1000
);

DisplayClock display
(
  CHSV (0x00, 0x00, 0x00),
  CHSV (0x00, 0x00, 0x80),
  CHSV (0x00, 0xFF, 0x80)
);


void setupDisplay()
{
  FastLED.addLeds<NEOPIXEL, pinLed>(leds, Layout::size());

  FastLED.setDither(0);
  FastLED.setBrightness(128);
}


void updateDisplay()
{
  TimeElements datetime;
  breakTime(now(), datetime);

  DisplayClock::State state;

  if (stateReceiver == DCF77_STATE_PENDING) {
    renderExtra(state.layerExtra, EXTRA_BIRTHDAY);

    display.update(millis(), state, transitionWave);
  }
  else {
    if (   datetime.Month  == 12
        && (   datetime.Day == 11
            || datetime.Day == 31)
        && datetime.Hour   == 23
        && datetime.Minute == 59
        && datetime.Second >= 50) {
          
      // Countdown to
      //   Happy Birthday Vivi
      //   Happy New Year
      renderCountdown(state.layerExtra, 60 - datetime.Second);

      display.update(millis(), state, transitionBurn);
    }
    else {
      // Normal time display
      renderTime(state.layerTime, datetime.Hour, datetime.Minute);

      // Extra text display
      //   Happy Birthday Vivi
      //   Happy New Year
      if (datetime.Month == 1 && datetime.Day == 1)
        renderExtra(state.layerExtra, EXTRA_NEWYEAR);
      else if (datetime.Month == 12 && datetime.Day == 12)
        renderExtra(state.layerExtra, EXTRA_BIRTHDAY);

      display.update(millis(), state, transitionWave);
    }
  }

  for (Layer::Iterator iterator; iterator != Layout::size(); ++iterator)
    leds[iterator] = CRGB(display.color(iterator));
}


///////////////////////////////////////////////////////////////////////////////
//
//  printTime()
//

void printTime()
{
  TimeElements datetime;
  breakTime(now(), datetime);

  Serial.print(datetime.Year + 1970);

  Serial.print('-');

  if (datetime.Month < 10)
    Serial.print('0');
  Serial.print(datetime.Month);

  Serial.print('-');

  if (datetime.Day < 10)
    Serial.print('0');
  Serial.print(datetime.Day);

  Serial.print(' ');

  if (datetime.Hour < 10)
    Serial.print('0');
  Serial.print(datetime.Hour);

  Serial.print(':');

  if (datetime.Minute < 10)
    Serial.print('0');
  Serial.print(datetime.Minute);

  Serial.print(':');

  if (datetime.Second < 10)
    Serial.print('0');
  Serial.print(datetime.Second);
}


///////////////////////////////////////////////////////////////////////////////
//
//  Timers
//

Timer timerReceiverOn     (Timer::ONCE | Timer::STOPPED,  5000);
Timer timerReceiverAdjust (Timer::ONCE | Timer::STOPPED, 30000);

Timer timerDisplay (Timer::REPEAT | Timer::STARTED | Timer::IMMEDIATE,   10);
Timer timerStatus  (Timer::REPEAT | Timer::STOPPED | Timer::DELAYED,   1000);


///////////////////////////////////////////////////////////////////////////////
//
//  setup()
//

void setup()
{
  Serial.begin(9600);

  Serial.println(F("Created 2015 by Michael Buschbeck & Judith Havemann for Vivi Havemann's 38th birthday."));
  Serial.println();
  Serial.println(F("Send 'D' to toggle display updates."));
  Serial.println(F("Send 'R' to reset DCF77 receiver."));
  Serial.println();

  setupDisplay();

  pinMode(pinReceiverData, INPUT);
  pinMode(pinReceiverOff,  OUTPUT);

  Serial.println(F("DCF77 receiver initially switched off."));
  digitalWrite(pinReceiverOff, HIGH);
  timerReceiverOn.start();

  setupReceiver();

  if (Serial)
    timerStatus.start();
}


///////////////////////////////////////////////////////////////////////////////
//
//  loop()
//

void loop()
{
  static bool working;

  static uint16_t nFrames         = 0;
  static uint32_t durationWorking = 0;
  static uint32_t durationIdle    = 0;

  working = false;
  uint32_t const timeStart = micros();


  if (timerReceiverOn.due()) {
    working = true;

    Serial.println(F("DCF77 receiver switched on. (Send 'R' to reset.)"));
    digitalWrite(pinReceiverOff, LOW);

    if (stateReceiver == DCF77_STATE_IGNORED)
      stateReceiver = DCF77_STATE_PENDING;
  }


  if (processTimestamp) {
    working = true;

    processTimestamp = false;

    Serial.print(F("DCF77 time received: "));
    timestamp.print(true);
    Serial.println();

    if (timestamp.status == DCF77Timestamp::VALID) {
      if (stateReceiver == DCF77_STATE_IGNORED) {
        Serial.println(F("DCF77 time adjustment ignored per user request."));
      }
      else {
        Serial.println(F("DCF77 time adjustment scheduled in 30 seconds."));
        timerReceiverAdjust.start();
      }
    }
  }


  if (timerReceiverAdjust.due()) {
    if (stateReceiver == DCF77_STATE_IGNORED) {
      Serial.println(F("DCF77 time adjustment cancelled: Ignored per user request."));
    }
    else if (timestamp.status != DCF77Timestamp::VALID) {
      Serial.println(F("DCF77 time adjustment cancelled: Received timestamp is not valid anymore."));
    }
    else {
      Serial.println(F("DCF77 time adjustment applied."));

      setTime(
        timestamp.time.hour,
        timestamp.time.minute,
        30,
        timestamp.date.day,
        timestamp.date.month,
        timestamp.date.year + 2000
      );

      stateReceiver = DCF77_STATE_RECEIVED;
    }
  }


  if (timerDisplay.due()) {
    working = true;

    updateDisplay();
    FastLED.show();

    nFrames += 1;
  }


  if (timerStatus.due()) {
    working = true;

    printTime();
    Serial.print(F(", "));
    
    printSamples();
    Serial.print(F(", "));

    uint8_t const activity = (100 * durationWorking / (durationWorking + durationIdle));

    Serial.print(F("FPS: "));
    Serial.print(nFrames);
    Serial.print(F(", CPU: "));
    Serial.print(activity);
    Serial.println(F("%"));

    nFrames = 0;

    durationWorking = 0;
    durationIdle    = 0;

    if (Serial.available()) {
      uint8_t const input = Serial.read();

      switch (input) {
        case 'd':
        case 'D':
          if (timerDisplay.active()) {
            timerDisplay.stop();
            Serial.println(F("Display updates switched off. (Send 'D' to toggle.)"));
          }
          else {
            timerDisplay.start();
            Serial.println(F("Display updates switched on. (Send 'D' to toggle.)"));
          }
          break;
          
        case 'r':
        case 'R':
          Serial.println(F("DCF77 receiver switched off for reset per user request."));
          digitalWrite(pinReceiverOff, HIGH);
          timerReceiverOn.start();
          break;

        case 't':
        case 'T':
          // Test countdown to Happy Birthday Vivi
          setTime(23, 59, 40, 11, 12, 2015);
          stateReceiver = DCF77_STATE_IGNORED;
          break;
      }
    }
  }


  uint32_t const timeDone = micros();
  uint32_t const duration = timeDone - timeStart;

  if (working)
         durationWorking += duration;
    else durationIdle    += duration;
}
