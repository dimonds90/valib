/*
  CRC calculation class
  =====================

  This class provides following functionality:
  * Can work with any given polinomial up to 32 bit width
  * Can work with any kind of bitstream (8/14/16/32 bits big/low endian)
  * Can work with byte streams and bit streams

  This module provides 2 predefined constant classes for CRC16 and CRC32.

  Important note!!!
  -----------------
  Low endian streams MUST be aligned to stream word boundary
  (16 bit for 14 and 16 bit stream and 32 bit for 32 bit stream)
*/



#ifndef CRC_H
#define CRC_H

#include "defs.h"

#define POLY_CRC16 0x8005
#define POLY_CRC32 0x04C11DB7


class CRC;

extern const CRC crc16;
extern const CRC crc32;

class CRC
{
protected:
  uint32_t poly;
  uint8_t  power;
  uint32_t tbl[256];
  uint32_t tbl6[64];

public:
  CRC() {};
  CRC(uint32_t _poly, size_t _power)
  { init(_poly, _power); };

  void init(uint32_t poly, size_t power);

  /////////////////////////////////////////////////////////////////////////////
  // Pre- and post- CRC processing
  // Required because 32bit left-aligned CRC value is used internally.
  //
  // We may not do this if start and and crc values equal to 0.
  // We may not do this if we compare CRC result with left-aligned value.
  // We may not do this for 32bit CRC.

  inline uint32_t crc_init(uint32_t crc) const { return crc << (32 - power); }
  inline uint32_t crc_get(uint32_t crc)  const { return crc >> (32 - power); }

  /////////////////////////////////////////////////////////////////////////////
  // CRC primitives
  // __forceinline is required because VC6 sometimes does not inline these 
  // functions what leads to dramatic performance decrease.

  __forceinline uint32_t add_bits (uint32_t crc, uint32_t data, size_t bits) const;
  __forceinline uint32_t add_8    (uint32_t crc, uint32_t data) const;
  __forceinline uint32_t add_14   (uint32_t crc, uint32_t data) const;
  __forceinline uint32_t add_16   (uint32_t crc, uint32_t data) const;
  __forceinline uint32_t add_32   (uint32_t crc, uint32_t data) const;

  /////////////////////////////////////////////////////////////////////////////
  // Byte stream interface

  uint32_t calc(uint32_t crc, uint8_t *data, size_t bytes, int bs_type) const;

  uint32_t calc(uint32_t crc, uint8_t *data, size_t size) const;
  uint32_t calc_14be(uint32_t crc, uint16_t *data, size_t size) const;
  uint32_t calc_14le(uint32_t crc, uint16_t *data, size_t size) const;
  uint32_t calc_16be(uint32_t crc, uint16_t *data, size_t size) const;
  uint32_t calc_16le(uint32_t crc, uint16_t *data, size_t size) const;
  uint32_t calc_32be(uint32_t crc, uint32_t *data, size_t size) const;
  uint32_t calc_32le(uint32_t crc, uint32_t *data, size_t size) const;

  /////////////////////////////////////////////////////////////////////////////
  // Bit stream interface

  uint32_t calc_bits(uint32_t crc, uint8_t *data, size_t start_bit, size_t bits, int bs_type) const;

  uint32_t calc_bits(uint32_t crc, uint8_t *data, size_t start_bit, size_t bits) const;
  uint32_t calc_bits_14be(uint32_t crc, uint16_t *data, size_t start_bit, size_t bits) const;
  uint32_t calc_bits_14le(uint32_t crc, uint16_t *data, size_t start_bit, size_t bits) const;
  uint32_t calc_bits_16be(uint32_t crc, uint16_t *data, size_t start_bit, size_t bits) const;
  uint32_t calc_bits_16le(uint32_t crc, uint16_t *data, size_t start_bit, size_t bits) const;
  uint32_t calc_bits_32be(uint32_t crc, uint32_t *data, size_t start_bit, size_t bits) const;
  uint32_t calc_bits_32le(uint32_t crc, uint32_t *data, size_t start_bit, size_t bits) const;
};

///////////////////////////////////////////////////////////////////////////////
// CRC primitives (inline)

uint32_t 
CRC::add_bits(uint32_t crc, uint32_t data, size_t bits) const
{
  if (bits)
  {
    crc ^= (data << (32 - bits));
    while (bits--)
      if (crc & 0x80000000)
        crc = (crc << 1) ^ poly;
      else
        crc <<= 1;
  }
  return crc;
}

uint32_t
CRC::add_8(uint32_t crc, uint32_t data) const
{
  return (crc << 8) ^ tbl[(crc >> 24) ^ (data & 0xff)];
}

uint32_t
CRC::add_14(uint32_t crc, uint32_t data) const
{
  crc ^= data << 18;
  crc = (crc << 8) ^ tbl[crc >> 24];
  crc = (crc << 6) ^ tbl6[crc >> 26];
  return crc;
}

uint32_t
CRC::add_16(uint32_t crc, uint32_t data) const
{
  crc ^= data << 16;
  crc = (crc << 8) ^ tbl[crc >> 24];
  crc = (crc << 8) ^ tbl[crc >> 24];
  return crc;
}

uint32_t
CRC::add_32(uint32_t crc, uint32_t data) const
{
  crc ^= data;
  crc = (crc << 8) ^ tbl[crc >> 24];
  crc = (crc << 8) ^ tbl[crc >> 24];
  crc = (crc << 8) ^ tbl[crc >> 24];
  crc = (crc << 8) ^ tbl[crc >> 24];
  return crc;
}

///////////////////////////////////////////////////////////////////////////////
// Simple CRC16 table algorithm

inline uint16_t calc_crc(uint16_t crc, uint8_t *data, size_t bytes)
{
  static const uint16_t crc_lut[256] = 
  {
    0x0000,0x8005,0x800f,0x000a,0x801b,0x001e,0x0014,0x8011,
    0x8033,0x0036,0x003c,0x8039,0x0028,0x802d,0x8027,0x0022,
    0x8063,0x0066,0x006c,0x8069,0x0078,0x807d,0x8077,0x0072,
    0x0050,0x8055,0x805f,0x005a,0x804b,0x004e,0x0044,0x8041,
    0x80c3,0x00c6,0x00cc,0x80c9,0x00d8,0x80dd,0x80d7,0x00d2,
    0x00f0,0x80f5,0x80ff,0x00fa,0x80eb,0x00ee,0x00e4,0x80e1,
    0x00a0,0x80a5,0x80af,0x00aa,0x80bb,0x00be,0x00b4,0x80b1,
    0x8093,0x0096,0x009c,0x8099,0x0088,0x808d,0x8087,0x0082,
    0x8183,0x0186,0x018c,0x8189,0x0198,0x819d,0x8197,0x0192,
    0x01b0,0x81b5,0x81bf,0x01ba,0x81ab,0x01ae,0x01a4,0x81a1,
    0x01e0,0x81e5,0x81ef,0x01ea,0x81fb,0x01fe,0x01f4,0x81f1,
    0x81d3,0x01d6,0x01dc,0x81d9,0x01c8,0x81cd,0x81c7,0x01c2,
    0x0140,0x8145,0x814f,0x014a,0x815b,0x015e,0x0154,0x8151,
    0x8173,0x0176,0x017c,0x8179,0x0168,0x816d,0x8167,0x0162,
    0x8123,0x0126,0x012c,0x8129,0x0138,0x813d,0x8137,0x0132,
    0x0110,0x8115,0x811f,0x011a,0x810b,0x010e,0x0104,0x8101,
    0x8303,0x0306,0x030c,0x8309,0x0318,0x831d,0x8317,0x0312,
    0x0330,0x8335,0x833f,0x033a,0x832b,0x032e,0x0324,0x8321,
    0x0360,0x8365,0x836f,0x036a,0x837b,0x037e,0x0374,0x8371,
    0x8353,0x0356,0x035c,0x8359,0x0348,0x834d,0x8347,0x0342,
    0x03c0,0x83c5,0x83cf,0x03ca,0x83db,0x03de,0x03d4,0x83d1,
    0x83f3,0x03f6,0x03fc,0x83f9,0x03e8,0x83ed,0x83e7,0x03e2,
    0x83a3,0x03a6,0x03ac,0x83a9,0x03b8,0x83bd,0x83b7,0x03b2,
    0x0390,0x8395,0x839f,0x039a,0x838b,0x038e,0x0384,0x8381,
    0x0280,0x8285,0x828f,0x028a,0x829b,0x029e,0x0294,0x8291,
    0x82b3,0x02b6,0x02bc,0x82b9,0x02a8,0x82ad,0x82a7,0x02a2,
    0x82e3,0x02e6,0x02ec,0x82e9,0x02f8,0x82fd,0x82f7,0x02f2,
    0x02d0,0x82d5,0x82df,0x02da,0x82cb,0x02ce,0x02c4,0x82c1,
    0x8243,0x0246,0x024c,0x8249,0x0258,0x825d,0x8257,0x0252,
    0x0270,0x8275,0x827f,0x027a,0x826b,0x026e,0x0264,0x8261,
    0x0220,0x8225,0x822f,0x022a,0x823b,0x023e,0x0234,0x8231,
    0x8213,0x0216,0x021c,0x8219,0x0208,0x820d,0x8207,0x0202
  };

  for(size_t i = 0; i < bytes; i++)
    crc = crc_lut[data[i] ^ (crc >> 8)] ^ (crc << 8);

  return crc;
}

#endif
