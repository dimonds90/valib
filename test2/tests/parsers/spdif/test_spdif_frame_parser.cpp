/*
  SPDIFFrameParser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/spdif/spdif_header.h"

static const uint8_t good[][32] =
{
  // SPDIF/AC3
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x01, 0x00, 0x00, 0xc0,
    0x77, 0x0b, 0x00, 0x00, 0x43, 0x25, 0x00, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // SPDIF/DTS
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x0b, 0x00, 0x68, 0x1f,
    0xFE, 0x7F, 0x01, 0x80, 0x3C, 0xFC, 0xD2, 0x3E, 0xE0, 0x75, 0x3B, 0x0D, 0x09, 0x80, 0xDE, 0x7B },

  // SPDIF/DTS14
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x0b, 0x00, 0x68, 0x1f,
    0xFF, 0x1F, 0x00, 0xE8, 0xF0, 0x07, 0x3E, 0x3C, 0x9D, 0x34, 0x00, 0x1E, 0xEE, 0x34, 0x09, 0x00 },

  // Padded DTS
  { 0xFE, 0x7F, 0x01, 0x80, 0x3C, 0xFC, 0xD2, 0x3E, 0xE0, 0x75, 0x3B, 0x0D, 0x09, 0x80, 0xDE, 0x7B,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // Padded DTS14
  { 0xFF, 0x1F, 0x00, 0xE8, 0xF0, 0x07, 0x3E, 0x3C, 0x9D, 0x34, 0x00, 0x1E, 0xEE, 0x34, 0x09, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

static const Speakers good_spk[] =
{
  Speakers(FORMAT_SPDIF, MODE_5_1, 48000),
  Speakers(FORMAT_SPDIF, MODE_5_1, 48000),
  Speakers(FORMAT_SPDIF, MODE_5_1, 48000),
  Speakers(FORMAT_SPDIF, MODE_5_1, 48000),
  Speakers(FORMAT_SPDIF, MODE_5_1, 48000),
};

static const uint8_t bad[][32] =
{
  // null header
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // null subheader
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x01, 0x00, 0x00, 0xc0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // SPDIF/AC3 - wrong byteorder
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x01, 0x00, 0x00, 0xc0,
    0x0b, 0x77, 0x00, 0x00, 0x25, 0x43, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // SPDIF/AC3 - bad ac3 header
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x01, 0x00, 0x00, 0xc0,
    0x77, 0x0b, 0x00, 0x00, 0x52, 0xc2, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // SPDIF/AC3 - wrong type (dts)
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x0c, 0x00, 0x00, 0xc0,
    0x77, 0x0b, 0x00, 0x00, 0x43, 0x25, 0x00, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // SPDIF/AC3 - bad type (2)
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x02, 0x00, 0x00, 0xc0,
    0x77, 0x0b, 0x00, 0x00, 0x43, 0x25, 0x00, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // SPDIF/DTS - wrong byteorder
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x0b, 0x00, 0x68, 0x1f,
    0x7F, 0xFE, 0x80, 0x01, 0xFC, 0x3C, 0x3E, 0xD2, 0x75, 0xE0, 0x0D, 0x3B, 0x80, 0x09, 0x7B, 0xDE },

  // SPDIF/DTS14 - wrong byteorder
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0xf8, 0x1f, 0x4e, 0x0b, 0x00, 0x68, 0x1f,
    0x1F, 0xFF, 0xE8, 0x00, 0x07, 0xF0, 0x3C, 0x3E, 0x34, 0x9D, 0x1E, 0x00, 0x34, 0xEE, 0x00, 0x09 },

  // Padded DTS - wrong byteorder
  { 0x7F, 0xFE, 0x80, 0x01, 0xFC, 0x3C, 0x3E, 0xD2, 0x75, 0xE0, 0x0D, 0x3B, 0x80, 0x09, 0x7B, 0xDE,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

  // Padded DTS14 - wrong byteorder
  { 0x1F, 0xFF, 0xE8, 0x00, 0x07, 0xF0, 0x3C, 0x3E, 0x34, 0x9D, 0x1E, 0x00, 0x34, 0xEE, 0x00, 0x09,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

BOOST_AUTO_TEST_SUITE(spdif_frame_parser)

BOOST_AUTO_TEST_CASE(can_parse)
{
  SPDIFFrameParser parser;
  BOOST_CHECK(parser.can_parse(FORMAT_SPDIF));
}

BOOST_AUTO_TEST_CASE(sync_info)
{
  SyncInfo sinfo = SPDIFFrameParser().sync_info();

  for (int i = 0; i < array_size(good); i++)
    BOOST_CHECK(sinfo.sync_trie.is_sync(good[i]));

//  for (int i = 0; i < array_size(bad); i++)
//    BOOST_CHECK(!sinfo.sync_trie.is_sync(bad[i]));
}

BOOST_AUTO_TEST_CASE(check_header)
{
  FrameInfo finfo;
  SPDIFFrameParser parser;

  for (int i = 0; i < array_size(good); i++)
  {
    BOOST_CHECK_MESSAGE(parser.parse_header(good[i], &finfo), i);
    BOOST_CHECK_MESSAGE(good_spk[i] == finfo.spk, i);
  }

  for (int i = 0; i < array_size(bad); i++)
    BOOST_CHECK_MESSAGE(!parser.parse_header(bad[i]), i);
}

BOOST_AUTO_TEST_CASE(compare_headers)
{
  SPDIFFrameParser parser;

  // Compare equal good headers
  for (int i = 0; i < array_size(good); i++)
    BOOST_CHECK_MESSAGE(parser.compare_headers(good[i], good[i]), i);

  // Compare different good headers
  for (int i = 0; i < array_size(good); i++)
    for (int j = i+1; j < array_size(good); j++)
    {
      BOOST_CHECK_MESSAGE(!parser.compare_headers(good[i], good[j]), i << ", " << j);
      BOOST_CHECK_MESSAGE(!parser.compare_headers(good[j], good[i]), i << ", " << j);
    }

  // Compare good with bad
  for (int i = 0; i < array_size(good); i++)
    for (int j = 0; j < array_size(bad); j++)
    {
      BOOST_CHECK_MESSAGE(!parser.compare_headers(good[i], bad[j]), i << ", " << j);
      BOOST_CHECK_MESSAGE(!parser.compare_headers(bad[j], good[i]), i << ", " << j);
    }

  // Compare bad headers
  // Compare only different headers because headers with wrong byteorder
  // are considered as equal and it is not compare_header() issue.
  for (int i = 0; i < array_size(bad); i++)
    for (int j = 0; j < array_size(bad); j++)
      if (i != j)
        BOOST_CHECK_MESSAGE(!parser.compare_headers(bad[i], bad[j]), i << ", " << j);
}

BOOST_AUTO_TEST_SUITE_END()
