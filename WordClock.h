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


#ifndef WORDCLOCK_H
#define WORDCLOCK_H

#include <FastLED.h>


///////////////////////////////////////////////////////////////////////////////
//
//  pre-declarations
//

class Layer;


///////////////////////////////////////////////////////////////////////////////
//
//  Layout
//

class Layout
{
public:
  static constexpr uint8_t rows() { return 11; }
  static constexpr uint8_t cols() { return 11; }

  static constexpr uint8_t size() { return rows() * cols(); }
  
  // iCol =              0   1   2 ...  10  11
  //                     |   |   |       |   |
  // iRow =  0   index 110 111 112 ... 119 120
  // iRow =  1   index 109 108 107 ... 100  99
  // iRow =  2   index  88  89  90 ...  97  98
  // iRow =  3   index  87  86  85 ...  78  77
  //         :           :   :   :       :   :
  // iRow =  9   index  21  20  19 ...  12  11
  // iRow = 10   index   0   1   2 ...   9  10

  static constexpr uint8_t indexFromCoords(uint8_t const iRow, uint8_t const iCol)
  {
    return (rows() - iRow - 1) * cols() + (iRow & 1 ? (cols() - iCol - 1) : iCol);
  }
};


///////////////////////////////////////////////////////////////////////////////
//
//  Word
//

class Word
{
public:
  constexpr Word(uint8_t const iRow, uint8_t const iCol, uint8_t const length)
    : index (indexInit(iRow, iCol, length))
    , count (countInit(iRow, iCol, length))
  {}

  void render(Layer& layer) const;

private:
  uint8_t const index;
  uint8_t const count;

  static constexpr uint8_t indexInit(uint8_t const iRow, uint8_t const iCol, uint8_t const length)
  {
    return Layout::indexFromCoords(iRow, (iRow & 1 ? iCol + length - 1 : iCol));
  }

  static constexpr uint8_t countInit(uint8_t const iRow, uint8_t const iCol, uint8_t const length)
  {
    return length;
  }
};


///////////////////////////////////////////////////////////////////////////////
//
//  Layer
//

class Layer
{
public:
  uint8_t bitmap[(Layout::size() + 7) / 8];

  inline Layer()
    : bitmap{}
  {}

  operator bool() const;

  bool operator==(Layer const & layer) const;
  bool operator!=(Layer const & layer) const;

  void print() const;


  class Iterator
  {
  public:
    Iterator(uint8_t const index = 0);

    Iterator& operator++();
    Iterator& operator=(uint8_t const index);

    operator uint8_t() const;

    bool get(Layer const & layer) const;

  private:
    uint8_t index;
    uint8_t offset;
    uint8_t bitmask;
  };
};


///////////////////////////////////////////////////////////////////////////////
//
//  Transition
//

class Transition
{
public:
  Transition();

  void start(uint32_t const time);
  void stop();

  bool isIdle() const;
  bool isRunning() const;

  virtual void prepare(uint32_t const time) = 0;
  virtual CRGB color(Layer::Iterator const iterator, CRGB const colorFrom, CRGB const colorTo) const = 0;

  static uint16_t calcSpeed(uint16_t const duration);
  static uint8_t  calcAlpha(uint16_t const progress, uint16_t const speed);

protected:
  bool idle;

  uint32_t timeStart;
  uint16_t progressTotal;
};


///////////////////////////////////////////////////////////////////////////////
//
//  TransitionWave
//

class TransitionWave
  : public Transition
{
public:
  template<typename FuncPixelDelay, typename FuncPixelDuration>
  TransitionWave(CRGB const colorVia, FuncPixelDelay const funcPixelDelay, FuncPixelDuration const funcPixelDuration);

  virtual void prepare(uint32_t const time) override;
  virtual CRGB color(Layer::Iterator const iterator, CRGB const colorFrom, CRGB const colorTo) const override;

private:
  CRGB colorVia;
  CRGB colorViaMax;
  uint8_t lumaVia;
  uint8_t lumaViaMax;
  uint16_t inverseLumaViaMax;

  struct Pixel
  {
    uint16_t delay;
    uint16_t speed;
  };

  Pixel pixels[Layout::size()];

  uint16_t durationTotal;
};


///////////////////////////////////////////////////////////////////////////////
//
//  Display
//

class Display
{
public:
  virtual CRGB color(Layer::Iterator const iterator) const = 0;
};


///////////////////////////////////////////////////////////////////////////////
//
//  DisplayClock
//

class DisplayClock
  : public Display
{
public:
  DisplayClock(CRGB const colorOff, CRGB const colorTime, CHSV const colorExtra, Transition& transition);

  struct State
  {
    Layer layerTime;
    Layer layerExtra;

    bool operator==(State const & state) const;
    bool operator!=(State const & state) const;
  };

  void update(State const state, uint32_t const time);

  bool isAnimating() const;
  bool isTransitioning() const;

  virtual CRGB color(Layer::Iterator const iterator) const override;

private:
  CRGB const colorOff;
  CRGB const colorTime;
  CHSV const colorExtra;

  uint32_t time;

  State statePrev;
  State stateCurr;

  Transition& transition;

  CRGB color(State const & state, Layer::Iterator const iterator) const;
};


///////////////////////////////////////////////////////////////////////////////
//
//  Word (implementation)
//

__attribute__((always_inline))
inline void Word::render(Layer& layer) const
{
  uint8_t const offset = this->index / 8;

  uint8_t const bitnoStart = this->index % 8;
  uint8_t const bitnoEnd   = this->index % 8 + this->count;

  if (bitnoEnd < 8) {
    layer.bitmap[offset] |= ((1 << bitnoEnd) - 1) ^ ((1 << bitnoStart) - 1);
  }
  else {
    layer.bitmap[offset]     |= 0xFF ^ ((1 << (bitnoStart  )) - 1);
    layer.bitmap[offset + 1] |=        ((1 << (bitnoEnd - 8)) - 1);
  }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Layer (implementation)
//

inline Layer::operator bool() const
{
  for (uint8_t offset = 0; offset < sizeof(this->bitmap); ++offset)
    if (this->bitmap[offset] != 0)
      return true;
  return false;
}


inline bool Layer::operator==(Layer const & layer) const
{
  for (uint8_t offset = 0; offset < sizeof(this->bitmap); ++offset)
    if (this->bitmap[offset] != layer.bitmap[offset])
      return false;
  return true;
}


inline bool Layer::operator!=(Layer const & layer) const
{
  for (uint8_t offset = 0; offset < sizeof(this->bitmap); ++offset)
    if (this->bitmap[offset] != layer.bitmap[offset])
      return true;
  return false;
}


void Layer::print() const
{
  for (uint8_t iRow = 0; iRow < Layout::rows(); ++iRow) {
    for (uint8_t iCol = 0; iCol < Layout::cols(); ++iCol) {
      uint8_t const index = Layout::indexFromCoords(iRow, iCol);

      uint8_t const offset = index / 8;
      uint8_t const bitno  = index % 8;

      Serial.print((this->bitmap[offset] & (1 << bitno)) != 0 ? 'X' : '-');
    }

    Serial.println();
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Layer::Iterator (implementation)
//

inline Layer::Iterator::Iterator(uint8_t const index)
  : index   (index)
  , offset  (index >> 3)
  , bitmask (1 << (index & 7))
{}


inline Layer::Iterator::operator uint8_t() const
{
  return this->index;
}


inline Layer::Iterator& Layer::Iterator::operator++()
{
  this->index += 1;
  this->bitmask <<= 1;

  if (this->bitmask == 0) {
    this->offset += 1;
    this->bitmask = 1;
  }

  return *this;
}


inline Layer::Iterator& Layer::Iterator::operator=(uint8_t const index)
{
  this->index   = index;
  this->offset  = index >> 3;
  this->bitmask = (1 << (index & 7));

  return *this;
}


inline bool Layer::Iterator::get(Layer const & layer) const
{
  return ((layer.bitmap[this->offset] & this->bitmask) != 0);
}


///////////////////////////////////////////////////////////////////////////////
//
//  Transition (implementation)
//

inline Transition::Transition()
  : idle (true)
{}


inline void Transition::start(uint32_t const time)
{
  this->idle = false;
  this->timeStart = time;
}


inline void Transition::stop()
{
  this->idle = true;
}


inline bool Transition::isIdle() const
{
  return this->idle;
}


inline bool Transition::isRunning() const
{
  return !this->idle;
}


inline uint16_t Transition::calcSpeed(uint16_t const duration)
{
  return static_cast<uint16_t>((static_cast<uint32_t>(65280) + duration - 1) / duration);
}


inline uint8_t Transition::calcAlpha(uint16_t const progress, uint16_t const speed)
{
  if (progress == 0)
    return 0;

  uint32_t alpha256 = static_cast<uint32_t>(progress) * speed;

  if (alpha256 > 65280)
    return 255;

  return static_cast<uint16_t>(alpha256 >> 8);
}


///////////////////////////////////////////////////////////////////////////////
//
//  TransitionWave (implementation)
//

template<typename FuncPixelDelay, typename FuncPixelDuration>
inline TransitionWave::TransitionWave(CRGB const colorVia, FuncPixelDelay const funcPixelDelay, FuncPixelDuration const funcPixelDuration)
{
  this->durationTotal = 0;

  for (uint8_t iRow = 0; iRow < Layout::rows(); ++iRow) {
    for (uint8_t iCol = 0; iCol < Layout::cols(); ++iCol) {
      uint8_t const index = Layout::indexFromCoords(iRow, iCol);

      uint16_t delay    = funcPixelDelay   (iRow, iCol);
      uint16_t duration = funcPixelDuration(iRow, iCol);

      this->pixels[index].delay = delay;
      this->pixels[index].speed = calcSpeed(duration);

      if (this->durationTotal < delay + duration)
        this->durationTotal = delay + duration;
    }
  }

  this->colorVia    = colorVia;
  this->colorViaMax = colorVia;
  this->colorViaMax.maximizeBrightness();

  this->lumaVia    = this->colorVia   .getLuma();
  this->lumaViaMax = this->colorViaMax.getLuma();

  this->inverseLumaViaMax = calcSpeed(this->lumaViaMax);
}


void TransitionWave::prepare(uint32_t const time)
{
  if (this->idle)
    return;

  uint16_t progressTotal = time - this->timeStart;

  if (progressTotal <= this->durationTotal) {
    this->progressTotal = progressTotal;
  }
  else {
    this->stop();
  }
}


CRGB TransitionWave::color(Layer::Iterator const iterator, CRGB const colorFrom, CRGB const colorTo) const
{
  if (this->idle)
    return colorTo;

  Pixel pixel = this->pixels[iterator];

  if (this->progressTotal < pixel.delay)
    return colorFrom;

  uint16_t progressPixel = this->progressTotal - pixel.delay;
  uint8_t alpha = calcAlpha(progressPixel, pixel.speed);

  if (alpha == 255)
    return colorTo;

  CRGB colorPeak;
  
  CRGB colorBoth (
    max(colorFrom.red,   colorTo.red),
    max(colorFrom.green, colorTo.green),
    max(colorFrom.blue,  colorTo.blue)
  );

  if (!colorBoth) {
    colorPeak = this->colorVia;
  }
  else {
    uint16_t lumaPeak = colorBoth.getLuma() + this->lumaVia;

    colorPeak = this->colorViaMax;

    if (lumaPeak < this->lumaViaMax) {
      uint8_t alpha = calcAlpha(lumaPeak, this->inverseLumaViaMax);
      //colorPeak.nscale8(static_cast<uint32_t>(lumaPeak) * 255 / this->lumaViaMax);
      colorPeak.nscale8(alpha);
    }
  }

  if (alpha < 128)
         return blend(colorFrom, colorPeak,  alpha        * 2    );
    else return blend(colorPeak, colorTo,   (alpha - 128) * 2 + 1);
}


///////////////////////////////////////////////////////////////////////////////
//
//  DisplayClock (implementation)
//

inline DisplayClock::DisplayClock(CRGB const colorOff, CRGB const colorTime, CHSV const colorExtra, Transition& transition)
  : colorOff   (colorOff)
  , colorTime  (colorTime)
  , colorExtra (colorExtra)
  , time       (0)
  , transition (transition)
{}


inline void DisplayClock::update(State const state, uint32_t const time)
{
  this->time = time;

  if (state != this->stateCurr) {
    this->statePrev = this->stateCurr;
    this->stateCurr = state;

    this->transition.start(time);
  }

  if (this->transition.isRunning())
    this->transition.prepare(time);
}


inline bool DisplayClock::isAnimating() const
{
  return (this->transition.isRunning() || this->stateCurr.layerExtra);
}


inline bool DisplayClock::isTransitioning() const
{
  return this->transition.isRunning();
}


CRGB DisplayClock::color(Layer::Iterator const iterator) const
{
  if (this->transition.isIdle()) {
    return this->color(this->stateCurr, iterator);
  }
  else {
    CRGB const colorFrom = this->color(this->statePrev, iterator);
    CRGB const colorTo   = this->color(this->stateCurr, iterator);

    return this->transition.color(iterator, colorFrom, colorTo);
  }
}


CRGB DisplayClock::color(State const & state, Layer::Iterator const iterator) const
{
  if (iterator.get(state.layerTime))
    return this->colorTime;

  if (iterator.get(state.layerExtra)) {
    // TODO rainbow animation:
    // TODO   per_pixel_phase = ((iRow + iCol) * 16) % 256
    // TODO   color
    // TODO     H = (this->time / 16 - per_pixel_phase) % 256
    // TODO     S = 0x80  <- take from this->colorExtra
    // TODO     V = 0x60  <- take from this->colorExtra

    uint8_t phasePixel = iterator * 16;

    return CHSV (
      this->time / 16 - phasePixel,
      this->colorExtra.saturation,
      this->colorExtra.value
    );
  }

  return this->colorOff;
}


///////////////////////////////////////////////////////////////////////////////
//
//  DisplayClock::State (implementation)
//

inline bool DisplayClock::State::operator==(DisplayClock::State const & state) const
{
  return (this->layerTime  == state.layerTime
       && this->layerExtra == state.layerExtra);
}


inline bool DisplayClock::State::operator!=(DisplayClock::State const & state) const
{
  return (this->layerTime  != state.layerTime
       || this->layerExtra != state.layerExtra);
}


#endif
