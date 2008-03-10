#include <math.h>
#include <string.h>
#include "param_ir.h"
#include "../dsp/kaiser.h"

inline double sinc(double x) { return x == 0 ? 1 : sin(x)/x; }
inline double lpf(int i, double f) { return 2 * f * sinc(i * 2 * M_PI * f); }


ParamIR::ParamIR():
ver(0), type(0), f1(0.0), f2(0.0), df(0.0), a(0.0), norm(false)
{}

ParamIR::ParamIR(int _type, double _f1, double _f2, double _df, double _a, bool _norm):
ver(0), type(_type), f1(_f1), f2(_f2), df(_df), a(_a), norm(_norm)
{}

void
ParamIR::set(int _type, double _f1, double _f2, double _df, double _a, bool _norm)
{
  ver++;
  type = _type;
  f1 = _f1;
  f2 = _f2;
  df = _df;
  a  = _a;
  norm = _norm;

  if (type == IR_BAND_PASS || type == IR_BAND_STOP)
    if (f1 > f2)
    {
      double temp = f1;
      f1 = f2; f2 = temp;
    }
}

void
ParamIR::get(int *_type, double *_f1, double *_f2, double *_df, double *_a, bool *_norm)
{
  if (_type) *_type = type;
  if (_f1)   *_f1 = f1;
  if (_f2)   *_f2 = f2;
  if (_df)   *_df = df;
  if (_a)    *_a  = a;
  if (_norm) *_norm = norm;
}

int
ParamIR::version() const
{ 
  return ver; 
}

ir_type
ParamIR::get_type(int sample_rate) const
{
  if (f1 < 0.0 || f2 < 0.0 || df <= 0.0 || a < 0.0) return ir_err;
  if (a == 0.0) return ir_identity;

  double nyquist = norm? 0.5: double(sample_rate) / 2;
  switch (type)
  {
    case IR_LOW_PASS:
      if (f1 >= nyquist) return ir_identity;
      return ir_custom;

    case IR_HIGH_PASS:
      return ir_custom;

    case IR_BAND_PASS:
      return ir_custom;

    case IR_BAND_STOP:
      if (f1 >= nyquist) return ir_identity;
      return ir_custom;

    default: return ir_err;
  }
}

int
ParamIR::min_length(int sample_rate) const
{
  if (norm)
    return kaiser_n(a, double(df)) | 1;
  else
    return kaiser_n(a, double(df) / sample_rate) | 1;
}

int
ParamIR::get_filter(int sample_rate, int n, sample_t *filter) const
{
  int i;
  int n_ = (n - 1) | 1; // make odd (type 1 filter)
  int c_ = n_ / 2;

  double f1_ = f1;
  double f2_ = f2;
  double df_ = df;
  double alpha = kaiser_alpha(a);

  if (!norm)
    f1_ /= sample_rate, f2_ /= sample_rate, df_ /= sample_rate;

  memset(filter, 0, n * sizeof(sample_t));
  filter[0] = 1.0;

  switch (type)
  {
    case IR_LOW_PASS:
      for (i = 0; i < n_; i++)
        filter[i] = (sample_t) (2 * f1_ * sinc((i - c_) * 2 * M_PI * f1_) * kaiser_window(i - c_, n_, alpha));
      return c_;

    case IR_HIGH_PASS:
      for (i = 0; i < n_; i++)
        filter[i] = (sample_t) (-2 * f1_ * sinc((i - c_) * 2 * M_PI * f1_) * kaiser_window(i - c_, n_, alpha));
      filter[c_] = (sample_t) ((1 - 2 * f1_) * kaiser_window(0, n_, alpha));
      return c_;

    case IR_BAND_PASS:
      for (i = 0; i < n_; i++)
        filter[i] = (sample_t) 
          filter[i] = (sample_t) ((2 * f2_ * sinc((i - c_) * 2 * M_PI * f2_) - 2 * f1_ * sinc((i - c_) * 2 * M_PI * f1_)) * kaiser_window(i - c_, n_, alpha));
      return c_;

    case IR_BAND_STOP:
      for (i = 0; i < n_; i++)
        filter[i] = (sample_t) 
          filter[i] = (sample_t) ((2 * f1_ * sinc((i - c_) * 2 * M_PI * f1_) - 2 * f2_ * sinc((i - c_) * 2 * M_PI * f2_)) * kaiser_window(i - c_, n_, alpha));
      filter[c_] = (sample_t) ((2 * f1_ + 1 - 2 * f2_) * kaiser_window(0, n_, alpha));
      return c_;  
  };
  return 0;
}
