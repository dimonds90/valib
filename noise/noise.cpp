#include <stdio.h>

#include "source\noise.h"
#include "filters\filter_chain.h"
#include "sink\sink_dsound.h"


int main()
{
  Speakers    spk(FORMAT_PCM16, MODE_STEREO, 48000);

  Noise       noise(spk, spk.sample_rate * spk.nch() * spk.sample_size() / 10);
  DSoundSink  ds(0);
  FilterChain filter;

  Sink *sink = &ds;

  /////////////////////////////////////////////////////////
  // Setup output
  /////////////////////////////////////////////////////////

  printf("Opening %s %s %iHz audio output...\n", spk.format_text(), spk.mode_text(), spk.sample_rate);
  if (!ds.open(spk))
  {
    printf("Error: Cannot open audio output!");
    return 1;
  }

  /////////////////////////////////////////////////////////
  // Process
  /////////////////////////////////////////////////////////

  Chunk chunk;
  do {
    if (!noise.get_chunk(&chunk))
    {
      printf("noise.get_chunk() failed\n");
      return 1;
    }

    if (chunk.is_eos())
      chunk.set_eos(true);

    if (!sink->process(&chunk))
    {
      printf("sink.process() failed\n");
      return 1;
    }
  } while (!chunk.is_eos());

  return 0;
}