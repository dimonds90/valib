/*
  Logging class
*/

#ifndef VALIB_LOG_H
#define VALIB_LOG_H

#define LOG_SCREEN 1 // print log at screen
#define LOG_HEADER 2 // print timestamp header
#define LOG_STATUS 4 // show status information

#define MAX_LOG_LEVELS 128

#include "auto_file.h"
#include "vtime.h"

class Log
{
protected:
  int level;
  int errors[MAX_LOG_LEVELS];
  vtime_t time[MAX_LOG_LEVELS];

  int flags;
  int istatus;
  vtime_t period;
  vtime_t tstatus;

  AutoFile f;

  void clear_status();
  void print_header(int _level);

public:
  Log(int flags = LOG_SCREEN | LOG_HEADER | LOG_STATUS, const char *log_file = 0, vtime_t period = 0.1);

  void open_group(const char *msg, ...);
  int close_group();

  int get_level();
  int get_errors();
  int get_total_errors();

  void msg(const char *msg, ...);
  int  err(const char *msg, ...);
  void status(const char *msg, ...);
};


#endif
