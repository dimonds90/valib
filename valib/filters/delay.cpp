#include <string.h>
#include "delay.h"

// optimize: use circular buffering instead of block-switching
// it should work better for large delays
// optimize: use larger buffer delay to gain advantages of circualr 
// buffer? (2 memcpy() calls instead of 2 memcpy and 1 memmove())
// how large should it be? use something like set_prebuffer()?
// do not forget about time lag in this case...

const float sonic_speed = 330; // [m/s]

Delay::Delay()
{
  enabled = false;
  units = DELAY_SP;
  memset(delays, 0, sizeof(delays));

  reset();
}

double     
Delay::units2samples(int _units)
{
  switch (_units)
  {
    case DELAY_SP: return 1.0;
    case DELAY_MS: return double(spk.sample_rate) /*[Hz]*/ / 1000.0; /*[ms/sec]*/
    case DELAY_M:  return -1.0 * double(spk.sample_rate) /*[Hz]*/ / sonic_speed; /*[m/s]*/
    case DELAY_CM: return -1.0 * double(spk.sample_rate) /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 100.0; /*[cm/m]*/
    case DELAY_FT: return -1.0 * double(spk.sample_rate) /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 3.28;  /*[ft/m]*/
    case DELAY_IN: return -1.0 * double(spk.sample_rate) /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 39.37; /*[in/m]*/
  }
  return 1.0;
}


int  
Delay::get_units() const
{
  return units;
}

void 
Delay::set_units(int _units)
{
  double factor = units2samples(units) / units2samples(_units);

  for (int ch = 0; ch < NCHANNELS; ch++)
    delays[ch] = float(delays[ch] * factor);
  units = _units;
}

void    
Delay::get_delays(float _delays[NCHANNELS]) const
{
  memcpy(_delays, delays, sizeof(delays));
}

void    
Delay::set_delays(float _delays[NCHANNELS])
{
  memcpy(delays, _delays, sizeof(delays));
  reset();
}

void 
Delay::reset()
{
  NullFilter::reset();

  int ch;
  int nch = spk.nch();
  const int *order = spk.order();

  double factor = units2samples(units);
  memset(ch_delays, 0, sizeof(ch_delays));
  for (ch = 0; ch < nch; ch++)
    ch_delays[ch] = int(delays[order[ch]] * factor);

  size_t nsamples = ch_delays[0];
  for (ch = 1; ch < nch; ch++)
    if (nsamples < ch_delays[ch])
      nsamples = ch_delays[ch];

  buf.allocate(nch, nsamples * 2);
  buf.zero();
  first_half = true;
}

bool 
Delay::process(const Chunk *_chunk)
{
  if (!receive_chunk(_chunk))
    return false;

  if (!enabled)
    return true; 

  size_t delay;
  sample_t *ptr1;
  sample_t *ptr2;
  sample_t *s;

  for (int ch = 0; ch < spk.nch(); ch++) 
    if (ch_delays[ch])
    {
      s = samples[ch];
      delay = ch_delays[ch];
      if (first_half)
      {
        ptr1 = buf[ch] + delay;
        ptr2 = buf[ch];
      }
      else
      {
        ptr1 = buf[ch];
        ptr2 = buf[ch] + delay;
      }

      if (size > delay)
      {
        memcpy(ptr2, s + size - delay, delay * sizeof(sample_t));
        memmove(s + delay, s, (size - delay) * sizeof(sample_t));
        memcpy(s, ptr1, delay * sizeof(sample_t));
      }
      else
      {
        memcpy(ptr2 + delay - size, s, size * sizeof(sample_t));
        memcpy(s, ptr1, size * sizeof(sample_t));
        memcpy(ptr2, ptr1 + size, (delay - size) * sizeof(sample_t));
      }
    }
  first_half = !first_half;
  return true;
}
