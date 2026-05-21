#include "GxEPD2_740c_E2741QS0B3.h"

#define ENABLE_LOGGING  0
#if ENABLE_LOGGING && __has_include("logging.h") 
#include "logging.h"
#else
#define LOG(format, ...)
#define LOG_RAW(format, ...)
#endif

GxEPD2_740c_E2741QS0B3::GxEPD2_740c_E2741QS0B3(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 50000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _paged = false;
  _use_fast_update = useFastFullUpdate;
}

void GxEPD2_740c_E2741QS0B3::selectFastFullUpdate(bool ff)
{
  if (ff != _use_fast_update)
  {
    _use_fast_update = ff;
    _InitDisplay();
  }
}

void GxEPD2_740c_E2741QS0B3::clearScreen(uint8_t value)
{
  clearScreen(value, 0xFF);
}

void GxEPD2_740c_E2741QS0B3::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  refresh();
}

void GxEPD2_740c_E2741QS0B3::writeScreenBuffer(uint8_t value)
{
  writeScreenBuffer(value, 0xFF);
}

void GxEPD2_740c_E2741QS0B3::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  //Serial.println("writeScreenBuffer");
  if (!_init_display_done) _InitDisplay();
  _writeCommand(0x10);
  _startTransfer();
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 4; i++)
  {
    _transfer(0xFF == black_value ? 0x55 : 0x00);
  }
  _endTransfer();
  _initial_write = false; // initial full screen buffer clean done
}

void GxEPD2_740c_E2741QS0B3::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if (hasPartialUpdate)
  {
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    int16_t x1 = x < 0 ? 0 : x; // limit
    int16_t y1 = y < 0 ? 0 : y; // limit
    int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
    int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
    if ((w1 <= 0) || (h1 <= 0)) return;
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 8; j++)
      {
        uint8_t data = 0xFF;
        // use wb, h of bitmap for index!
        uint32_t idx = mirror_y ? j + dx / 8 + uint32_t((h - 1 - (i + dy))) * wb : j + dx / 8 + uint32_t(i + dy) * wb;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
#else
          data = bitmap[idx];
#endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                          (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
          data <<= 4;
          _transfer(data2);
        }
      }
    }
    _endTransfer();
  }
  else
  {
    if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
    {
      //Serial.println("paged");
      _startTransfer();
      for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 8; i++)
      {
        uint8_t data = bitmap[i];
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                          (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
          data <<= 4;
          _transfer(data2);
        }
      }
      _endTransfer();
      if (y + h == HEIGHT) // last page
      {
        //Serial.println("paged ended");
        _paged = false;
      }
    }
    else
    {
      _paged = false;
      int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
      x -= x % 8; // byte boundary
      w = wb * 8; // byte boundary
      if ((w <= 0) || (h <= 0)) return;
      _writeCommand(0x10);
      _startTransfer();
      for (int16_t i = 0; i < int16_t(HEIGHT); i++)
      {
        for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
        {
          uint8_t data = 0xFF;
          if ((j >= x) && (j <= x + w) && (i >= y) && (i < y + h))
          {
            uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
            if (pgm)
            {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
              data = pgm_read_byte(&bitmap[idx]);
#else
              data = bitmap[idx];
#endif
            }
            else
            {
              data = bitmap[idx];
            }
            if (invert) data = ~data;
          }
          for (int16_t k = 0; k < 2; k++)
          {
            uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                            (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
            data <<= 4;
            _transfer(data2);
          }
        }
      }
      _endTransfer();
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_740c_E2741QS0B3::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!black && !color) return;
  if (!color) return writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  //Serial.print("writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if (hasPartialUpdate)
  {
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    int16_t x1 = x < 0 ? 0 : x; // limit
    int16_t y1 = y < 0 ? 0 : y; // limit
    int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
    int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
    if ((w1 <= 0) || (h1 <= 0)) return;
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 8; j++)
      {
        uint8_t black_data = 0xFF, color_data = 0xFF;
        // use wb, h of bitmap for index!
        uint32_t idx = mirror_y ? j + dx / 8 + uint32_t((h - 1 - (i + dy))) * wb : j + dx / 8 + uint32_t(i + dy) * wb;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          black_data = pgm_read_byte(&black[idx]);
          color_data = pgm_read_byte(&color[idx]);
#else
          black_data = black[idx];
          color_data = color[idx];
#endif
        }
        else
        {
          black_data = black[idx];
          color_data = color[idx];
        }
        if (invert)
        {
          black_data = ~black_data;
          color_data = ~color_data;
        }
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t out_data = 0x00;
          for (int16_t l = 0; l < 4; l++)
          {
            out_data <<= 2;
            if (!(color_data & 0x80)) out_data |= 0x03; // red
            else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
            black_data <<= 1;
            color_data <<= 1;
          }
          _transfer(out_data);
        }
      }
    }
    _endTransfer();
  }
  else
  {
    if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
    {
      //Serial.println("paged");
      _startTransfer();
      for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 8; i++)
      {
        uint8_t black_data = black[i];
        uint8_t color_data = color[i];
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t out_data = 0x00;
          for (int16_t l = 0; l < 4; l++)
          {
            out_data <<= 2;
            if (!(color_data & 0x80)) out_data |= 0x03; // red
            else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
            black_data <<= 1;
            color_data <<= 1;
          }
          _transfer(out_data);
        }
      }
      _endTransfer();
      if (y + h == HEIGHT) // last page
      {
        //Serial.println("paged ended");
        _paged = false;
      }
    }
    else
    {
      _paged = false;
      int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
      x -= x % 8; // byte boundary
      w = wb * 8; // byte boundary
      if ((w <= 0) || (h <= 0)) return;
      _writeCommand(0x10);
      _startTransfer();
      for (int16_t i = 0; i < int16_t(HEIGHT); i++)
      {
        for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
        {
          uint8_t black_data = 0xFF, color_data = 0xFF;
          if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
          {
            uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
            if (pgm)
            {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
              black_data = pgm_read_byte(&black[idx]);
              color_data = pgm_read_byte(&color[idx]);
#else
              black_data = black[idx];
              color_data = color[idx];
#endif
            }
            else
            {
              black_data = black[idx];
              color_data = color[idx];
            }
            if (invert)
            {
              black_data = ~black_data;
              color_data = ~color_data;
            }
          }
          for (int16_t k = 0; k < 2; k++)
          {
            uint8_t out_data = 0x00;
            for (int16_t l = 0; l < 4; l++)
            {
              out_data <<= 2;
              if (!(color_data & 0x80)) out_data |= 0x03; // red
              else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
              black_data <<= 1;
              color_data <<= 1;
            }
            _transfer(out_data);
          }
        }
      }
      _endTransfer();
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_740c_E2741QS0B3::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImagePart("); Serial.print(x_part); Serial.print(", "); Serial.print(y_part); Serial.print(", ");
  //Serial.print(w_bitmap); Serial.print(", "); Serial.print(h_bitmap); Serial.print(", ");
  //Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  // make x, w multiple of 8
  w += x % 8; // adjust for byte boundary of x
  x -= x % 8; // byte boundary ox
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (hasPartialUpdate)
  {
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 8; j++)
      {
        uint8_t data;
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? x_part / 8 + j + dx / 8 + uint32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + uint32_t(y_part + i + dy) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
#else
          data = bitmap[idx];
#endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                          (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
          data <<= 4;
          _transfer(data2);
        }
      }
    }
    _endTransfer();
  }
  else
  {
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
      {
        uint8_t data = 0xFF;
        if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
        {
          // use wb_bitmap, h_bitmap of bitmap for index!
          uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&bitmap[idx]);
#else
            data = bitmap[idx];
#endif
          }
          else
          {
            data = bitmap[idx];
          }
          if (invert) data = ~data;
        }
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                          (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
          data <<= 4;
          _transfer(data2);
        }
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_740c_E2741QS0B3::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImagePart("); Serial.print(x_part); Serial.print(", "); Serial.print(y_part); Serial.print(", ");
  //Serial.print(w_bitmap); Serial.print(", "); Serial.print(h_bitmap); Serial.print(", ");
  //Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  if (!black && !color) return;
  if (!color) return writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  // make x, w multiple of 8
  w += x % 8; // adjust for byte boundary of x
  x -= x % 8; // byte boundary ox
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (hasPartialUpdate)
  {
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 8; j++)
      {
        uint8_t black_data = 0xFF, color_data = 0xFF;
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? x_part / 8 + j + dx / 8 + uint32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + uint32_t(y_part + i + dy) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          black_data = pgm_read_byte(&black[idx]);
          color_data = pgm_read_byte(&color[idx]);
#else
          black_data = black[idx];
          color_data = color[idx];
#endif
        }
        else
        {
          black_data = black[idx];
          color_data = color[idx];
        }
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t out_data = 0x00;
          for (int16_t l = 0; l < 4; l++)
          {
            out_data <<= 2;
            if (!(color_data & 0x80)) out_data |= 0x03; // red
            else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
            black_data <<= 1;
            color_data <<= 1;
          }
          _transfer(out_data);
        }
      }
    }
    _endTransfer();
  }
  else
  {
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
      {
        uint8_t black_data = 0xFF, color_data = 0xFF;
        if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
        {
          // use wb_bitmap, h_bitmap of bitmap for index!
          uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            black_data = pgm_read_byte(&black[idx]);
            color_data = pgm_read_byte(&color[idx]);
#else
            black_data = black[idx];
            color_data = color[idx];
#endif
          }
          else
          {
            black_data = black[idx];
            color_data = color[idx];
          }
          if (invert)
          {
            black_data = ~black_data;
            color_data = ~color_data;
          }
        }
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t out_data = 0x00;
          for (int16_t l = 0; l < 4; l++)
          {
            out_data <<= 2;
            if (!(color_data & 0x80)) out_data |= 0x03; // red
            else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
            black_data <<= 1;
            color_data <<= 1;
          }
          _transfer(out_data);
        }
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_740c_E2741QS0B3::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!data1) return;
  //Serial.print("writeNative("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if (hasPartialUpdate)
  {
    int16_t wb = (w + 3) / 4; // width bytes, bitmaps are padded
    x -= x % 4; // byte boundary
    w = wb * 4; // byte boundary
    int16_t x1 = x < 0 ? 0 : x; // limit
    int16_t y1 = y < 0 ? 0 : y; // limit
    int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
    int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
    if ((w1 <= 0) || (h1 <= 0)) return;
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 4; j++)
      {
        uint8_t data;
        // use wb, h of bitmap for index!
        uint32_t idx = mirror_y ? j + dx / 4 + uint32_t((h - 1 - (i + dy))) * wb : j + dx / 4 + uint32_t(i + dy) * wb;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&data1[idx]);
#else
          data = data1[idx];
#endif
        }
        else
        {
          data = data1[idx];
        }
        //if (invert) data = ~data;
        if (invert) data = _invert(data);
        _transfer(data);
      }
    }
    _endTransfer();
  }
  else
  {
    if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
    {
      //Serial.println("paged");
      _startTransfer();
      for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 4; i++)
      {
        uint8_t data = data1[i];
        _transfer(data);
      }
      _endTransfer();
      if (y + h == HEIGHT) // last page
      {
        //Serial.println("paged ended");
        _paged = false;
      }
    }
    else
    {
      //Serial.println("not paged");
      _paged = false;
      int16_t wb = (w + 3) / 4; // width bytes, bitmaps are padded
      x -= x % 4; // byte boundary
      w = wb * 4; // byte boundary
      if ((w <= 0) || (h <= 0)) return;
      _writeCommand(0x10);
      _startTransfer();
      for (int16_t i = 0; i < int16_t(HEIGHT); i++)
      {
        for (int16_t j = 0; j < int16_t(WIDTH); j += 4)
        {
          uint8_t data = 0x55;
          if (data1)
          {
            if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
            {
              uint32_t idx = mirror_y ? (j - x) / 4 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 4 + uint32_t(i - y) * wb;
              if (pgm)
              {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                data = pgm_read_byte(&data1[idx]);
#else
                data = data1[idx];
#endif
              }
              else
              {
                data = data1[idx];
              }
              //if (invert) data = ~data;
              if (invert) data = _invert(data);
            }
          }
          _transfer(data);
        }
      }
      _endTransfer();
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_740c_E2741QS0B3::writeNativePart(const uint8_t* data1, const uint8_t* data2, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!data1) return;
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (!_init_display_done) _InitDisplay();
  if (_initial_write) writeScreenBuffer();
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 3) / 4; // width bytes, bitmaps are padded
  x_part -= x_part % 4; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  w += x % 4; // adjust for byte boundary of x
  x -= x % 4; // byte boundary
  w = 4 * ((w + 3) / 4); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (hasPartialUpdate)
  {
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 4; j++)
      {
        uint8_t data;
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? x_part / 4 + j + dx / 4 + uint32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 4 + j + dx / 4 + uint32_t(y_part + i + dy) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&data1[idx]);
#else
          data = data1[idx];
#endif
        }
        else
        {
          data = data1[idx];
        }
        //if (invert) data = ~data;
        if (invert) data = _invert(data);
        _transfer(data);
      }
    }
    _endTransfer();
  }
  else
  {
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 4)
      {
        uint8_t data = 0x55;
        if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
        {
          // use wb_bitmap, h_bitmap of bitmap for index!
          uint32_t idx = mirror_y ? (x_part + j - x1) / 4 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 4 + uint32_t(y_part + i - y1) * wb_bitmap;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&data1[idx]);
#else
            data = data1[idx];
#endif
          }
          else
          {
            data = data1[idx];
          }
          //if (invert) data = ~data;
          if (invert) data = _invert(data);
        }
        _transfer(data);
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_740c_E2741QS0B3::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_740c_E2741QS0B3::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_740c_E2741QS0B3::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_740c_E2741QS0B3::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_740c_E2741QS0B3::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_740c_E2741QS0B3::refresh(bool partial_update_mode)
{
  if (hasPartialUpdate) _setPartialRamArea(0, 0, WIDTH, HEIGHT);
  _refresh(partial_update_mode);
}

void GxEPD2_740c_E2741QS0B3::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  // intersection with screen
  int16_t w1 = x < 0 ? w + x : w; // reduce
  int16_t h1 = y < 0 ? h + y : h; // reduce
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  w1 = x1 + w1 < int16_t(WIDTH) ? w1 : int16_t(WIDTH) - x1; // limit
  h1 = y1 + h1 < int16_t(HEIGHT) ? h1 : int16_t(HEIGHT) - y1; // limit
  if ((w1 <= 0) || (h1 <= 0)) return;
  // make x1, w1 multiple of 4
  w1 += x1 % 4;
  if (w1 % 4 > 0) w1 += 4 - w1 % 4;
  x1 -= x1 % 4;
  bool fullscreen = ((x1 == 0) && (y1 == 0) && (w1 == WIDTH) && (h1 == HEIGHT));
  if (hasPartialUpdate) _setPartialRamArea(x1, y1, w1, h1, !fullscreen);
  _refresh(!fullscreen);
}

void GxEPD2_740c_E2741QS0B3::powerOff()
{
  _PowerOff();
}

void GxEPD2_740c_E2741QS0B3::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData(0xA5);    // control code
    _hibernating = true;
    _init_display_done = false;
  }
}

void GxEPD2_740c_E2741QS0B3::setPaged()
{
  if (!hasPartialUpdate)
  {
    _paged = true;
    _InitDisplay();
    _writeCommand(0x10);
  }
}

void GxEPD2_740c_E2741QS0B3::_refresh(bool partial_update_mode)
{
  _writeCommand(0x50); // VCOM and Data Interval Setting
  _writeData(partial_update_mode ? 0x97 : 0x37); // floating or white
  _writeCommand(0x12); // Display Refresh
  _writeData(0x00);
  delay(1);
  _waitWhileBusy("_refresh", full_refresh_time);
  //_power_is_on = false; // not needed
  _init_display_done = false; // needed
}

void GxEPD2_740c_E2741QS0B3::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial_mode)
{
  //Serial.print("_setPartialRamArea("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  _writeCommand(0x83);
  _writeData(x / 256);
  _writeData(x % 256);
  _writeData((x + w - 1) / 256);
  _writeData((x + w - 1) % 256);
  _writeData(y / 256);
  _writeData(y % 256);
  _writeData((y + h - 1) / 256);
  _writeData((y + h - 1) % 256);
  _writeData(partial_mode ? 0x01 : 0x00);
}

void GxEPD2_740c_E2741QS0B3::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_740c_E2741QS0B3::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x02);
    _writeData(0x00);
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
}

void GxEPD2_740c_E2741QS0B3::_InitDisplay()
{
  //Serial.println("_InitDisplay");
  // initcode from Good Dispay demo for GDEY029F51H
  if ((_rst >= 0) && (_hibernating || _initial_write))
  {
     LOG("_rst %d _busy %d _cs %d _dc %d\n",_rst,_busy,_cs,_dc);
    digitalWrite(_rst, HIGH);
    delay(20); // At least 20ms delay
    digitalWrite(_rst, LOW); // Module reset
    delay(2);  // At least 40ms delay, 2ms for WS "clever" reset
    digitalWrite(_rst, HIGH);
    delay(2); // At least 50ms delay (32ms measured)
    _waitWhileBusy("_InitDisplay reset", power_on_time);
    LOG("Busy inactive after inital reset\n");

    _hibernating = false;
    _power_is_on = false;
  }
  E2741Q_init();
  _PowerOn();
  _init_display_done = true;
}

uint8_t GxEPD2_740c_E2741QS0B3::_invert(uint8_t data)
{
  uint8_t result = 0x00;
  for (int i = 0; i < 4; i++)
  {
    switch (data & 0x03)
    {
      case 0x00 : result |= 0x01 << 6; break;
      case 0x01 : result |= 0x02 << 6; break;
      case 0x02 : result |= 0x03 << 6; break;
      case 0x03 : result |= 0x00 << 6; break;
    }
    if (i == 3) break;
    data >>= 2;
    result >>= 2;
  }
  return result;
}

void WaitBusy(uint8_t State);

#define FUNCT_END()  0,0
#define FUNCT_WAIT_BUSY(x)  0,1,x

// <num_bytes>
// 
// two possible formats
// if <num_bytes> == zero then the next bytes is the function to call
//    function 0:  end of list
//    function 1: FUNCT_WAIT_BUSY(x,y) were 
//                x wait for busy == x
//                y wait timeout in seconds
// otherwise <num_bytes> is the number of bytes to the EPD, the first byte is 
// sent as a command and the rest are sent as data
static const uint8_t epd_E2741Q_init[] PROGMEM = {
   2,0xE6,0x19,
   2,0xE0,0x02,
   1,0xA5,

   FUNCT_WAIT_BUSY(1),
// (PWR): Power setting Register ?? JD79665AA shows 6 parameters
   2,0x01,0x07,
   3,0x00,  //  (PSR): Panel setting Register
   0x07, // 00 0 0 0 0 1 1 1 
         // 00 RES: - 800 x 600
         // 0 PST_MODE: Power switching time in the period of frame scanning.
         // 0 X
         // 0 Ud: 0xScan down
         // 1 SHL: shift left
         // 1 SHD_N: Booster on
         // 1 RST_N: not reset
   0xAB, // 1 0 1 0 1 0 1 1
         // 1 LUT_EN: Using LUT from register
         // 0 X
         // 1 FOPT: No scan after waveform finished and switch the source channel output to Hiz.
         // 0 VCMZ 0: VCOM status function: no effect
         // 1 TS_AUTO: When RST_N low to high,Temperature Sensor will be activated automatically one time. (
         // 0 TIEG: VGN power off status function 0 no effect
         // 1 NORG: After refreshing display, VCOM is tied to GND before power off
         // 1 VC_LUTZ: After refreshing display, the output of VCOM is set to floating automatically

   5,0x61,     // TRES
   0x01,0xE0,  // 0x1e0: 480
   0x03,0x20,  // 0x320: 800

   3,0x00,     //PSB
   0x07,
      //  00 RES: - 800 x 600
      //  0 PST_MODE: Power switching time in the period of frame scanning.
      //  0 X
      //  0 Ud: 0xScan down
      //  1 SHL: shift left
      //  1 SHD_N: Booster on
      //  1 RST_N: not reset

   0x2B,   // 0 0 1 0 1 0 1 1
      // 0 LUT_EN: Using LUT from MTP
      // 0 X
      // 1 FOPT: No scan after waveform finished and switch the source channel output to Hiz.
      // 0 VCMZ 0: VCOM status function: no effect
      // 1 TS_AUTO: When RST_N low to high,Temperature Sensor will be activated automatically one time. (
      // 0 TIEG: VGN power off status function 0 no effect
      // 1 NORG: After refreshing display, VCOM is tied to GND before power off
      // 1 VC_LUTZ: After refreshing display, the output of VCOM is set to floating automatically

   4,0x06,  // BTST
   0x40,0x40,0x40,

   4,0x03,   // ?? not defined for JD79665AA perhaps POFS (various JD devices
   0x00,0x00,0x00,

   2,0xE7,  // not defined for JD79665AA perhaps SPI2 enable (ST7789)
   0x3C,

   5,0x65,      //(GSST): Gate/Source Start Setting Register
   0x00,0x00,0x00,0x00,

   2,0x30,   // (PLL): PLL Control Register
   0x08,
#if 0
// Done in _refresh based on partial_update_mode
   2,0x50,  //  (CDI): VCOM and DATA interval setting Register
   0x37,
#endif

   3,0x60,   // ?? not defined for JD79665AA perhaps TCON ? (various controllers
   0x03,0x03,

   2,0xE3,   // (PWS): Power Saving Register ??? JD79665AA shows 2 argments
   0x00,

   2,0xFF,   // ?? not defined for JD79665AA
   0xA5,

   9,0xEF,    // not defined for JD79665AA
   0x01,0x1E,0x06,0x0A,0x0F,0x19,0x0F,0x09,

   2,0xDC,  // ?? not defined for JD79665AA perhaps RDID3 ?
   0x01,

   2,0xDD,  // ?? not defined for JD79665AA 
   0x04,

   2,0xDE,  //?? not defined for JD79665AA  
   0x01,

   2,0xE8,  // ?? not defined for JD79665AA perhaps PWCTRL2 ?
   0x01,

   2,0xDA,  // ?? not defined for JD79665AA perhaps ILI9341_RDID1
   0x3F,

   2,0xFF,  // ?? not defined for JD79665AA
   0xE3,

   2,0xE9,  // ?? not defined for JD79665AA perhaps ST7789_EQCTRL
   0x01,
   FUNCT_END()
};


void GxEPD2_740c_E2741QS0B3::RunSequence(const uint8_t *p)
{
   uint8_t Count;
   uint8_t bRun = 1;
   while(bRun) {
      if((Count = *p++) == 0) {
      // function
         switch(*p++) {
            case 0:  // FUNCT_END
               bRun = 0;
               break;

            case 1:  // FUNCT_WAIT_BUSY(state)
               LOG("Calling WaitBusy\n");
               WaitBusy(*p++);
               break;
         }
      }
      else {
         LOG_RAW("\nC: %02X\n",*p);
         _writeCommand(*p++);
         while(--Count) {
            LOG_RAW("D: %02X\n",*p);
            _writeData(*p++);
         }
      }
   }
}

void GxEPD2_740c_E2741QS0B3::WaitBusy(uint8_t State)
{
   volatile int Busy;
   const char *DesiredState = State ? "high" : "low";

   pinMode(4,INPUT);
   Busy = digitalRead(4);

   LOG("Waiting for busy %s, starting with %d\n",DesiredState,Busy);
   do {
      Busy = digitalRead(4);
   } while(Busy != State);
   LOG("Busy went %s\n",DesiredState);
}

void GxEPD2_740c_E2741QS0B3::E2741Q_init()
{
   RunSequence(epd_E2741Q_init);
}

void GxEPD2_740c_E2741QS0B3::E2741Q_wakeup()
{
   E2741Q_init();

}

