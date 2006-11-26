#ifndef SPDIF_WRAPPER_H
#define SPDIF_WRAPPER_H

#include "parser.h"
#include "multi_header.h"
#include "spdif_frame.h"

class SPDIFWrapper : public FrameParser
{
public:
  enum dts_mode_enum { dts_wrapped, dts_headerless, dts_auto };

  dts_mode_enum dts_mode;
  bool use_dts14;

  SPDIFWrapper(dts_mode_enum dts_mode = dts_auto, bool use_dts14 = false);
  ~SPDIFWrapper();

  HeaderInfo header_info() const { return hdr; }

  /////////////////////////////////////////////////////////
  // FrameParser overrides

  virtual const HeaderParser *header_parser() const;

  virtual void reset();
  virtual bool parse_frame(uint8_t *frame, size_t size);

  virtual Speakers  get_spk()      const { return spk;          }
  virtual samples_t get_samples()  const { samples_t samples; samples.zero(); return samples; }
  virtual size_t    get_nsamples() const { return hdr.nsamples; }
  virtual uint8_t  *get_rawdata()  const { return spdif_frame;  }
  virtual size_t    get_rawsize()  const { return spdif_size;   }

  virtual size_t stream_info(char *buf, size_t size) const;
  virtual size_t frame_info(char *buf, size_t size) const;

protected:
  uint8_t    *buf;          // output frame buffer

  MultiHeader spdifable;    // spdifable formats header parser
  SPDIFFrame  spdif_parser; // required to rewrap spdif input
  HeaderInfo  hdr;          // input raw frame info

  Speakers    spk;          // output format
  uint8_t    *spdif_frame;  // spdif frame pointer
  size_t      spdif_size;   // spdif frame size
  
  // Note, that this structure differs from the similar structure at SPDIFFrame
  // and SPDIFHeader (it includes zero fields). It is because SPDIFHeader and
  // SPDIFFrame are required to work with incorrectly wrapped streams, but
  // SPDIFWrapper must generate correct one.
  //
  // Because of this we alse have to always reparse spdif input: frame size
  // reported by SPDIFHeader (and received with parse_frame) is less than 
  // required for correct passthrough and initial zeros are skipped.
  //
  // SPDIF reparsing also allows to convert between different DTS output modes
  // even when we have SPDIF input.

  struct spdif_header_s
  {
    uint16_t zero1;
    uint16_t zero2;
    uint16_t zero3;
    uint16_t zero4;

    uint16_t sync1;   // Pa sync word 1
    uint16_t sync2;   // Pb sync word 2
    uint16_t type;    // Pc data type
    uint16_t len;     // Pd length-code (bits)

    inline void set(uint16_t _type, size_t _len_bits)
    {
      zero1 = 0;
      zero2 = 0;
      zero3 = 0;
      zero4 = 0;

      sync1 = 0xf872;
      sync2 = 0x4e1f;
      type  = _type;
      len   = _len_bits;
    }
  };

};

#endif