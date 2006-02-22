#include <string.h>
#include <spk.h>
#include <log.h>

#include <helpers\syncscan.h>
#include <source\noise.h>
#include <win32\cpu.h>

SyncScan s;

static const int syncwords[] = 
{
  // ac3
  0x0b770000,
  0x770b0000,
  // dts
  0xff1f00e8,
  0x1fffe800,
  0xfe7f0180,
  0x7ffe8001,
};
const max_syncwords = array_size(syncwords);


static bool mpa_le_sync(const uint8_t *_buf)
{
  if ((_buf[0] == 0xff)         && // sync
     ((_buf[1] & 0xf0) == 0xf0) && // sync
     ((_buf[1] & 0x06) != 0x00) && // layer
     ((_buf[2] & 0xf0) != 0xf0) && // biterate
     ((_buf[2] & 0x0c) != 0x0c))   // sample rate
    return true;
  else
    return false;
}

static bool mpa_be_sync(const uint8_t *_buf)
{
  if ((_buf[1] == 0xff)         && // sync
     ((_buf[0] & 0xf0) == 0xf0) && // sync
     ((_buf[0] & 0x06) != 0x00) && // layer
     ((_buf[3] & 0xf0) != 0xf0) && // biterate
     ((_buf[3] & 0x0c) != 0x0c))   // sample rate
    return true;
  else
    return false;
}

static bool mpa_sync(const uint8_t *_buf)
{
  return mpa_le_sync(_buf) || mpa_be_sync(_buf);
}

static bool ac3_sync(const uint8_t *_buf)
{
  // 8 bit or 16 bit little endian steram sync
  if ((_buf[0] == 0x0b) && (_buf[1] == 0x77))
    return true;
  // 16 bit big endian steram sync
  else if ((_buf[1] == 0x0b) && (_buf[0] == 0x77))
    return true;
  else 
    return false;
}

static bool dts_sync(const uint8_t *_buf)
{
  // 14 bits little endian bitstream
  if (_buf[0] == 0xff && _buf[1] == 0x1f &&
      _buf[2] == 0x00 && _buf[3] == 0xe8)
    return true;
  // 14 bits big endian bitstream
  else if (_buf[0] == 0x1f && _buf[1] == 0xff &&
           _buf[2] == 0xe8 && _buf[3] == 0x00)
    return true;
  // 16 bits little endian bitstream
  else if (_buf[0] == 0xfe && _buf[1] == 0x7f &&
           _buf[2] == 0x01 && _buf[3] == 0x80)
    return true;
  // 16 bits big endian bitstream
  else if (_buf[0] == 0x7f && _buf[1] == 0xfe &&
           _buf[2] == 0x80 && _buf[3] == 0x01)
    return true;
  else
  // no sync
    return false;
}

static bool is_sync(const uint8_t *_buf)
{
  return ac3_sync(_buf) || dts_sync(_buf) || mpa_sync(_buf);
}


int 
test_syncer(Log *log)
{
  log->open_group("Testing Syncer");

  int i;
  for (i = 0; i < array_size(syncwords); i++)
    s.set(i, syncwords[i], 0xffffffff);

  const int max_ptr_offset = 32;
  const int max_block_size = 32;
  const int max_offset = 128;
  const int buf_size = max_ptr_offset + max_block_size + max_offset + 16;

  uint8_t *buf = new uint8_t[buf_size];
  uint8_t scanbuf[4];

  //////////////////////////////////////////////////////////
  // Syncpoint find test

  for (int isyncword = 0; isyncword < max_syncwords; isyncword++)
  {
    log->status("syncword: %x", syncwords[isyncword]);
    for (int ptr_offset = 0; ptr_offset < max_ptr_offset; ptr_offset++)
      for (int block_size = 1; block_size < max_block_size; block_size++)
        for (int offset = 0; offset < max_offset; offset++)
        {
          memset(buf, 0, buf_size);
          *(uint32_t *)(buf + ptr_offset + offset) = swab_u32(syncwords[isyncword]);
          s.reset();

          for (int i = 0; i < offset + 16; i += block_size)
          {
            int gone = s.scan(buf + i + ptr_offset, block_size);

            if (s.get_sync())
            {
              if (gone > block_size)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Too much bytes gone: %i", gone);
                break;
              }

              if (i + gone - 4 != offset)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Sync found at %i", i+gone-4);
              }

              if (s.count != 4)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Count = %i", s.count);
              }

              if ((s.get_sync() & (1 << isyncword)) == 0)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Wrong sync = %i", s.count);
              }
              break;
            }
            else
              if (gone != block_size)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Wrong number of bytes gone: %i", gone);
                break;
              }

          }
          if (!s.get_sync())
          {
            log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
            log->err("Sync was not found");
          }
        }
  }

  //////////////////////////////////////////////////////////
  // Syncpoint find test

  for (isyncword = 0; isyncword < max_syncwords; isyncword++)
  {
    log->status("syncword: %x", syncwords[isyncword]);
    for (int ptr_offset = 0; ptr_offset < max_ptr_offset; ptr_offset++)
      for (int block_size = 1; block_size < max_block_size; block_size++)
        for (int offset = 0; offset < max_offset; offset++)
        {
          memset(buf, 0, buf_size);
          *(uint32_t *)(buf + ptr_offset + offset) = swab_u32(syncwords[isyncword]);
          s.reset();

          for (int i = 0; i < offset + 16; i += block_size)
          {
            int gone = s.scan(scanbuf, buf + i + ptr_offset, block_size);

            if (s.get_sync(scanbuf))
            {
              if (gone > block_size)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Too much bytes gone: %i", gone);
                break;
              }

              if (i + gone - 4 != offset)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Sync found at %i", i+gone-4);
              }

              if ((s.get_sync(scanbuf) & (1 << isyncword)) == 0)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Wrong sync = %i", s.count);
              }
              break;
            }
            else
              if (gone != block_size)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Wrong number of bytes gone: %i", gone);
                break;
              }

          }
          if (!s.get_sync(scanbuf))
          {
            log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
            log->err("Sync was not found");
          }
        }
  }

  delete buf;

  //////////////////////////////////////////////////////////
  // MPA sync test

  s.clear_all();
  s.set_standard(SYNCMASK_MPA_LE);

  uint32_t syncword;
  int mpa_allowed = 0;

  for (syncword = 0xfff00000; syncword < 0xffffffff; syncword++)
  {
    *(uint32_t *)scanbuf = swab_u32(syncword);
    if (s.get_sync(scanbuf))
    {
      mpa_allowed++;
      if (!mpa_le_sync(scanbuf))
        break;
    }
    else
    {
      if (mpa_le_sync(scanbuf))
        break;
    }
  }

  if (syncword < 0xffffffff)
  {
    if (s.get_sync(scanbuf))
      log->err("0x%08x is detected as MPA sync", syncword);
    else
      log->err("0x%08x is not detected as MPA sync", syncword);
  }
  else
    log->msg("Correct MPA headers found: %i", mpa_allowed);

  //////////////////////////////////////////////////////////
  // Speed test

  s.clear_all();
  s.set_standard(SYNCMASK_MAD);

  const int runs = 50;
  const int size = 10000000;
  const int syncpoints = 1683;

  Chunk chunk;
  Noise noise(spk_unknown, size, size);
  noise.get_chunk(&chunk);

  CPUMeter cpu;
  int sync_count;

  sync_count = 0;
  cpu.reset();
  cpu.start();
  s.reset();
  for (i = 0; i < runs; i++)
  {
    s.reset();
    size_t gone = 0;
    while (gone < chunk.size)
    {
      gone += s.scan(chunk.rawdata + gone, chunk.size - gone);
      if (s.get_sync())
        if (is_sync(s.syncbuf))
          sync_count++;
        else
        {
          log->err("0x%08x is detected as syncpoint", swab_u32(s.syncword));
          break;
        }
    }
  }
  cpu.stop();

  if (sync_count != syncpoints * runs)
    log->err("Syncpoints found: %i (must be %i)", sync_count / runs, syncpoints);

  log->msg("Sync scan speed: %iMB/s, Syncpoints found: %i", 
    int(double(chunk.size) * runs / cpu.get_thread_time() / 1000000), 
    sync_count / runs);

  sync_count = 0;
  cpu.reset();
  cpu.start();
  memset(scanbuf, 0, sizeof(scanbuf));
  for (i = 0; i < runs; i++)
  {
    s.reset();
    size_t gone = 0;
    while (gone < chunk.size)
    {
      gone += s.scan(scanbuf, chunk.rawdata + gone, chunk.size - gone);
      if (s.get_sync(scanbuf))
        if (is_sync(scanbuf))
          sync_count++;
        else
        {
          log->err("0x%08x is detected as syncpoint", swab_u32(s.syncword));
          break;
        }
    }
  }
  cpu.stop();

  if (sync_count != syncpoints * runs)
    log->err("Syncpoints found: %i (must be %i)", sync_count / runs, syncpoints);

  log->msg("Sync scan speed: %iMB/s, Syncpoints found: %i", 
    int(double(chunk.size) * runs / cpu.get_thread_time() / 1000000), 
    sync_count / runs);


  return log->close_group();
};