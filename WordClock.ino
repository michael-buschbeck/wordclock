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

#define SDA_PIN 6
#define SDA_PORT PORTD
#define SCL_PIN 7
#define SCL_PORT PORTD
#define I2C_FASTMODE 1
#include <SoftI2CMaster.h>

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

constexpr uint8_t pinLed               = A0;  // OUTPUT

constexpr uint8_t pinClockReceiverData = A1;  // INPUT
constexpr uint8_t pinClockReceiverOff  = A2;  // OUTPUT

constexpr uint8_t pinClockRealtimeGND  =  4;  // OUTPUT, LOW
constexpr uint8_t pinClockRealtimeVCC  =  5;  // variable
constexpr uint8_t pinClockRealtimeSDA  =  6;  // variable (port D, pin 4)
constexpr uint8_t pinClockRealtimeSCL  =  7;  // variable (port D, pin 5)

constexpr uint8_t i2cClockRealtime = 0b01101000;


///////////////////////////////////////////////////////////////////////////////
//
//  DCF77 Receiver
//

DCF77Timestamp timestamp;
volatile bool processTimestamp = false;


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


void setupClockReceiver()
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
  bool const sample = (digitalRead(pinClockReceiverData) == 0 ? true : false);

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
  CHSV (0x10, 0xFF, 0x80)
);


void setupDisplay()
{
  FastLED.addLeds<NEOPIXEL, pinLed>(leds, Layout::size());

  FastLED.setDither(0);
  FastLED.setBrightness(128);
}


void updateDisplayIntro()
{
  DisplayClock::State state;

  renderExtra(state.layerExtra, EXTRA_BIRTHDAY);

  display.update(millis(), state, transitionWave);
}


void updateDisplayTime()
{
  TimeElements datetime;
  breakTime(now(), datetime);

  DisplayClock::State state;

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


void updateLeds()
{
  for (Layer::Iterator iterator; iterator != Layout::size(); ++iterator)
    leds[iterator] = CRGB(display.color(iterator));

  FastLED.show();
}


///////////////////////////////////////////////////////////////////////////////
//
//  DS3231 Real-Time Clock
//

constexpr bool useClockRealtimeBatteryWhileIdle = false;

bool connectedClockRealtime = false;


void setupClockRealtime()
{
  // Ground reference
  pinMode(pinClockRealtimeGND, OUTPUT);
  digitalWrite(pinClockRealtimeGND, LOW);

  // Input voltage (use VCC during I2C initialization)
  pinMode(pinClockRealtimeVCC, OUTPUT);
  digitalWrite(pinClockRealtimeVCC, HIGH);

  // Wait until VCC has been established
  delayMicroseconds(10);

  // Verify I2C communication channel
  if (i2c_init() && i2c_start((i2cClockRealtime << 1) | I2C_WRITE)) {
    i2c_write(0x00);
    i2c_stop();
    connectedClockRealtime = true;
  }

  if (useClockRealtimeBatteryWhileIdle) {
    // Input voltage (use battery)
    pinMode(pinClockRealtimeVCC, INPUT);
  }
}


void writeClockRealtime(TimeElements const & datetime)
{
  uint8_t const second  = datetime.Second;
  uint8_t const minute  = datetime.Minute;
  uint8_t const hour    = datetime.Hour;
  uint8_t const weekday = datetime.Wday;
  uint8_t const day     = datetime.Day;
  uint8_t const month   = datetime.Month;
  uint8_t const year    = datetime.Year + (1970 - 2000);

  if (useClockRealtimeBatteryWhileIdle) {
    // Input voltage (use VCC during I2C communication)
    pinMode(pinClockRealtimeVCC, OUTPUT);
    digitalWrite(pinClockRealtimeVCC, HIGH);

    // Wait until VCC has been established
    delayMicroseconds(10);
  }

  // Initiate I2C write communication and write register address
  i2c_start((i2cClockRealtime << 1) | I2C_WRITE);
  i2c_write(0x00);

  // Write all time and date registers
  i2c_write(((second / 10) << 4) | (second % 10));
  i2c_write(((minute / 10) << 4) | (minute % 10));
  i2c_write(((hour   / 10) << 4) | (hour   % 10));
  i2c_write(weekday);
  i2c_write(((day    / 10) << 4) | (day    % 10));
  i2c_write(((month  / 10) << 4) | (month  % 10));
  i2c_write(((year   / 10) << 4) | (year   % 10));

  // Stop I2C write communication
  i2c_stop();

  if (useClockRealtimeBatteryWhileIdle) {
    // Input voltage (use battery)
    pinMode(pinClockRealtimeVCC, INPUT);
  }
}


void readClockRealtime(TimeElements& datetime)
{
  if (useClockRealtimeBatteryWhileIdle) {
    // Input voltage (use VCC during I2C communication)
    pinMode(pinClockRealtimeVCC, OUTPUT);
    digitalWrite(pinClockRealtimeVCC, HIGH);

    // Wait until VCC has been established
    delayMicroseconds(10);
  }

  // Initiate I2C write communication and write register address
  i2c_start((i2cClockRealtime << 1) | I2C_WRITE);
  i2c_write(0x00);

  // Initiate I2C read communication starting from there
  i2c_rep_start((i2cClockRealtime << 1) | I2C_READ);

  // Read all time and date registers
  uint8_t const second  = i2c_read(false);
  uint8_t const minute  = i2c_read(false);
  uint8_t const hour    = i2c_read(false);
  uint8_t const weekday = i2c_read(false);
  uint8_t const day     = i2c_read(false);
  uint8_t const month   = i2c_read(false);
  uint8_t const year    = i2c_read(true);

  // Stop I2C read communication
  i2c_stop();

  if (useClockRealtimeBatteryWhileIdle) {
    // Input voltage (use battery)
    pinMode(pinClockRealtimeVCC, INPUT);
  }

  datetime.Second = (second >> 4) * 10 + (second & 0x0F);
  datetime.Minute = (minute >> 4) * 10 + (minute & 0x0F);
  datetime.Hour   = (hour   >> 4) * 10 + (hour   & 0x0F);
  datetime.Wday   = weekday;
  datetime.Day    = (day    >> 4) * 10 + (day    & 0x0F);
  datetime.Month  = (month  >> 4) * 10 + (month  & 0x0F);
  datetime.Year   = (year   >> 4) * 10 + (year   & 0x0F) + (2000 - 1970);
}


///////////////////////////////////////////////////////////////////////////////
//
//  Time
//

void printTime()
{
  TimeElements datetime;
  breakTime(now(), datetime);

  printTime(datetime);
}


void printTime(TimeElements const & datetime)
{
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


int32_t updateTime(TimeElements const & datetime)
{
  time_t const nowBefore = now();

  setTime(
    datetime.Hour,
    datetime.Minute,
    datetime.Second,
    datetime.Day,
    datetime.Month,
    datetime.Year + (1970 - 2000)
  );

  return static_cast<int32_t>(now() - nowBefore);
}


bool readTime(TimeElements& datetime)
{
  uint8_t digit10 = 0;

  for (uint8_t field = 0; field < (2+2+2 + 2+2); ) {
    int const input = Serial.read();

    // Abort on any non-digit input
    if (input < '0' || input > '9')
      return false;

    uint8_t const digit = (input - '0');
    
    switch (field++) {
      case 0:  // 10-digit of year
      case 2:  // 10-digit of month
      case 4:  // 10-digit of day
      case 6:  // 10-digit of hour
      case 8:  // 10-digit of minute
        digit10 = digit * 10;
        break;

      case 1:  // 1-digit of year
        datetime.Year = digit10 + digit + (2000 - 1970);
        break;

      case 3:  // 1-digit of month
        datetime.Month = digit10 + digit;
        if (datetime.Month < 1 || datetime.Month > 12)
          return false;
        break;

      case 5:  // 1-digit of day
        datetime.Day = digit10 + digit;
        if (datetime.Day < 1 || datetime.Day > 31)
          return false;
        break;

      case 7:  // 1-digit of hour
        datetime.Hour = digit10 + digit;
        if (datetime.Hour > 23)
          return false;
        break;

      case 9:  // 1-digit of minute
        datetime.Minute = digit10 + digit;
        if (datetime.Minute > 59)
          return false;
        break;
    }
  }

  // Second is not part of accepted input
  datetime.Second = 0;

  return true;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Timers
//

Timer timerClockReceiverOn     (Timer::ONCE   | Timer::STOPPED,         3UL * 1000UL);
Timer timerClockReceiverAdjust (Timer::ONCE   | Timer::STOPPED,        30UL * 1000UL);
Timer timerClockReceiverReset  (Timer::REPEAT | Timer::STARTED, 60UL * 60UL * 1000UL);

Timer timerDisplay (Timer::REPEAT | Timer::STOPPED | Timer::IMMEDIATE,   10UL);
Timer timerUpdate  (Timer::REPEAT | Timer::STOPPED | Timer::DELAYED,   1000UL);


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
  Serial.println(F("Send 'S' followed by 'yyMMddHHmm' timestamp to set date and time."));
  Serial.println();

  setupDisplay();

  pinMode(pinClockReceiverData, INPUT);
  pinMode(pinClockReceiverOff,  OUTPUT);

  Serial.println(F("DCF77 receiver initially switched off."));
  digitalWrite(pinClockReceiverOff, HIGH);
  timerClockReceiverOn.start();

  setupClockReceiver();

  setupClockRealtime();

  if (connectedClockRealtime) {
    Serial.println(F("DS3231 real-time clock communication established, setting on-board time."));

    TimeElements datetime;
    readClockRealtime(datetime);

    updateTime(datetime);
  }

  for (uint32_t timeIntroStart = millis(); millis() - timeIntroStart < 5000UL; ) {
    updateDisplayIntro();
    updateLeds();
  }

  timerDisplay.start();
  timerUpdate .start();
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


  if (timerClockReceiverOn.due()) {
    working = true;

    Serial.println(F("DCF77 receiver switched on. (Send 'R' to reset.)"));
    digitalWrite(pinClockReceiverOff, LOW);
  }


  if (processTimestamp) {
    working = true;

    processTimestamp = false;

    Serial.print(F("DCF77 time received: "));
    timestamp.print(true);
    Serial.println();

    if (timestamp.status == DCF77Timestamp::VALID) {
      Serial.println(F("DCF77 time adjustment scheduled in 30 seconds."));
      timerClockReceiverAdjust.start();
      timerClockReceiverReset .start();
    }
  }


  if (timerClockReceiverAdjust.due()) {
    working = true;

    if (timestamp.status != DCF77Timestamp::VALID) {
      Serial.println(F("DCF77 time adjustment cancelled: Received timestamp is not valid anymore."));
    }
    else {
      TimeElements datetime;

      datetime.Hour   = timestamp.time.hour;
      datetime.Minute = timestamp.time.minute;
      datetime.Second = 30;
      datetime.Day    = timestamp.date.day;
      datetime.Month  = timestamp.date.month;
      datetime.Year   = timestamp.date.year + (2000 - 1970);

      int32_t const deltaSeconds = updateTime(datetime);

      if (connectedClockRealtime)
        writeClockRealtime(datetime);

      if (deltaSeconds == 0) {
        Serial.println(F("DCF77 time adjustment applied (no delta against on-board time)."));
      }
      else {
        Serial.print(F("DCF77 time adjustment applied ("));
        printTime(datetime);
        Serial.print(F(", delta: "));
        Serial.print(deltaSeconds);
        Serial.println(F(" seconds against on-board time)."));
      }
    }
  }

  
  if (timerClockReceiverReset.due()) {
    working = true;

    Serial.println(F("DCF77 receiver switched off for automatic periodic reset."));
    digitalWrite(pinClockReceiverOff, HIGH);
    timerClockReceiverOn.start();
  }


  if (timerUpdate.due()) {
    working = true;

    if (connectedClockRealtime) {
      uint8_t const second = now() % 60;

      if (second == 45) {
        TimeElements datetime;
        readClockRealtime(datetime);

        int32_t const deltaSeconds = updateTime(datetime);

        if (deltaSeconds != 0) {
          Serial.print(F("On-board time adjusted against DS3231 real-time clock ("));
          printTime(datetime);
          Serial.print(F(", delta: "));
          Serial.print(deltaSeconds);
          Serial.println(F(" seconds)."));
        }
      }
    }

    if (Serial) {
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
    }
  }


  if (timerDisplay.due()) {
    working = true;

    updateDisplayTime();
    updateLeds();

    nFrames += 1;
  }


  if (Serial.available()) {
    working = true;

    int const input = Serial.read();

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
        digitalWrite(pinClockReceiverOff, HIGH);
        timerClockReceiverOn   .start();
        timerClockReceiverReset.start();
        break;

      case 's':
      case 'S':
        TimeElements datetime;

        if (readTime(datetime)) {
          int32_t const deltaSeconds = updateTime(datetime);

          if (connectedClockRealtime)
            writeClockRealtime(datetime);

          if (deltaSeconds == 0) {
            Serial.println(F("On-board time and RS3231 real-time clock adjusted per user request (no delta against on-board time)."));
          }
          else {
            Serial.print(F("On-board time and RS3231 real-time clock adjusted per user request ("));
            printTime(datetime);
            Serial.print(F(", delta: "));
            Serial.print(deltaSeconds);
            Serial.println(F(" seconds against on-board time)."));
          }
        }
        else {
          Serial.println(F("On-board time and RS3231 real-time clock not adjusted: Invalid timestamp input (expected 'yyMMddHHmm')."));
        }
        break;
    }
  }


  uint32_t const timeDone = micros();
  uint32_t const duration = timeDone - timeStart;

  if (working)
         durationWorking += duration;
    else durationIdle    += duration;
}
