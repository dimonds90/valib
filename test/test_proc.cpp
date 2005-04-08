/*
  AudioProcessor PCM passthrough test
  AudioProcessor should not alter input stream with default settings
*/

#include "source/raw_source.h"
#include "source/noise.h"
#include "filters/proc.h"
#include "common.h"

// PCM passthrough test
// (we cannot test FORMAT_PCMFLOAT because noise generates
// floats > 1.0)

int test_proc(Log *log)
{
  log->open_group("AudioProcessor PCM Noise passthrough test");
  static const int formats[] = 
  { 
    FORMAT_LINEAR, 
    FORMAT_PCM16,    FORMAT_PCM24,    FORMAT_PCM32,
    FORMAT_PCM16_LE, FORMAT_PCM24_LE, FORMAT_PCM32_LE,
  };

  static const sample_t levels[] = 
  {
    1.0,
    32768, 8388608, 2147483648,
    32768, 8388608, 2147483648
  };

  static const int modes[] = 
  { 
    MODE_1_0,     MODE_2_0,     MODE_3_0,     MODE_2_1,     MODE_3_1,     MODE_2_2,
    MODE_1_0_LFE, MODE_2_0_LFE, MODE_3_0_LFE, MODE_2_1_LFE, MODE_3_1_LFE, MODE_2_2_LFE,
  };

  int iformat = 0;
  int imode = 0;
  const int seed = 345346;
  const int nsamples = 8192;

  Speakers spk;
  Noise src;
  Noise ref;
  AudioProcessor proc(2048);

  for (iformat = 0; iformat < array_size(formats); iformat++)
    for (imode = 0; imode < array_size(modes); imode++)
    {
      spk.set(formats[iformat], modes[imode], 48000, levels[iformat]);
      log->msg("Testing %s %s %iHz with %iK samples", spk.format_text(), spk.mode_text(), spk.sample_rate, nsamples / 1024);

      if (!src.set_output(spk) || 
          !ref.set_output(spk) )
      {
        log->err("Cannot init noise source");
        continue;
      }
      if (spk.format == FORMAT_LINEAR)
      {
        src.set_data_size(nsamples);
        ref.set_data_size(nsamples);
      }
      else
      {
        src.set_data_size(nsamples * spk.nch() * spk.sample_size());
        ref.set_data_size(nsamples * spk.nch() * spk.sample_size());
      }
      src.set_seed(seed);
      ref.set_seed(seed);

      if (!proc.set_input(spk) || !proc.set_output(spk))
      {
        log->err("Cannot init processor");
        continue;
      }

      compare(log, &src, &proc, &ref);
    }
  return log->close_group();
}