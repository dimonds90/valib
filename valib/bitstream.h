/*
  Bitstream operations
  Read & Write classes

  Known issues:
  =============
  1) Low-endian streams must be aligned to 16 bits (for 16 bits stream) or 32 bits 
     (for 32 bits stream) boundary.

  2) We must not use ReadBS::get(0).
*/


#ifndef BITSTREAM_H              
#define BITSTREAM_H

#include "defs.h"

class ReadBS
{
private:
  const uint32_t *start;
  const uint32_t *pos;
  uint32_t  bits_left;
  uint32_t  current_word;
  int       type;      // BITSTREAM_XXXX constants
  
  inline void fill_current();
  uint32_t get_bh(uint32_t num_bits);
  int32_t  get_bh_signed(uint32_t num_bits);

public:
  void set_ptr(const uint8_t *buf, int type = BITSTREAM_8);

  inline uint32_t get(uint32_t num_bits);
  inline int32_t  get_signed(uint32_t num_bits);
  inline bool     get_bool();

  inline int get_type() const { return type; }
  int get_pos() const;
};


class WriteBS
{
public:
  uint32_t bit_buf;
  int      bit_left;

  uint8_t *buf;           // output buffer begin
  uint8_t *buf_end;       // output buffer end
  uint8_t *buf_ptr;       // current buffer pointer

  int32_t  data_out_size; // in bytes

public:
  void set_ptr(uint8_t *data, int data_size);

  void put_bits(int bits, int value);
  void put_bool(bool value);
  void flush();

  inline uint8_t *get_buf() const { return buf;     }
  inline uint8_t *get_end() const { return buf_end; }
  inline uint8_t *get_ptr() const { return buf_ptr; }
  int get_pos() const;
};


///////////////////////////////////////////////////////////////////////////////
// ReadBS inlines
//

inline void 
ReadBS::fill_current()
{
  uint32_t tmp;
  tmp = *(pos++);
  switch (type)
  {
    case BITSTREAM_8:
    case BITSTREAM_16BE: 
    case BITSTREAM_32BE: 
      current_word = swab_u32(tmp);
      bits_left = 32;
      break;

    case BITSTREAM_16LE:
      current_word = (tmp >> 16) | (tmp << 16);
      bits_left = 32;
      break;

    case BITSTREAM_14BE:
      tmp = swab_u32(tmp);
      current_word = (tmp & 0x3fff) | ((tmp & 0x3fff0000) >> 2);
      bits_left = 28;
      break;

    case BITSTREAM_14LE:
      tmp = (tmp >> 16) | (tmp << 16);
      current_word = (tmp & 0x3fff) | ((tmp & 0x3fff0000) >> 2);
      bits_left = 28;
      break;
  }
}

inline uint32_t 
ReadBS::get(uint32_t num_bits)
{
  uint32_t result;

  if (num_bits < bits_left) 
  {
    result = (current_word << (32 - bits_left)) >> (32 - num_bits);
    bits_left -= num_bits;
    return result;
  }

  return get_bh(num_bits);
}

inline int32_t 
ReadBS::get_signed(uint32_t num_bits)
{
  int32_t result;
        
  if (num_bits < bits_left) 
  {
    result = (((int32_t)current_word) << (32 - bits_left)) >> (32 - num_bits);
    bits_left -= num_bits;
    return result;
  }

  return get_bh_signed(num_bits);
}

inline bool 
ReadBS::get_bool()
{
  return get(1) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// WriteBS inlines
//

inline void
WriteBS::put_bits(int bits, int value)
{
  if (bits < bit_left) 
  {
    bit_buf = (bit_buf << bits) | value;
    bit_left -= bits;
  } 
  else 
  {
    bit_buf <<= bit_left;
    bit_buf |= value >> (bits - bit_left);
    *(uint32_t *)buf_ptr = swab_u32(bit_buf);
    buf_ptr  += 4;
    bit_left += 32 - bits;
    bit_buf   = value;
  }
}

inline void
WriteBS::put_bool(bool value)
{
  int v = value & 1;
  if (bit_left > 1) 
  {
    bit_buf = (bit_buf << 1) | v;
    bit_left--;
  } 
  else 
  {
    bit_buf = (bit_buf << 1) | v;
    *(uint32_t *)buf_ptr = swab_u32(bit_buf);
    buf_ptr  += 4;
    bit_left += 31;
    bit_buf   = value;
  }
}

#endif
