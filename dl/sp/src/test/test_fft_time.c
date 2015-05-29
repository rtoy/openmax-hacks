/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"

#define MAX_FFT_ORDER TWIDDLE_TABLE_ORDER

#define ENABLE_FIXED_POINT_FFT_TESTS

#if defined(FLOAT_ONLY) || defined(ARM_VFP_TEST)
/*
 * Fixed-point FFTs are disabled if we only want float tests or if
 * we're building for non-NEON tests.
 */
#undef ENABLE_FIXED_POINT_FFT_TESTS
#endif

typedef enum {
  S16,
  S32,
} s16_s32;


#define DEFINE_ONE_FFT(name) \
  void TimeOne ## name(int count, int fft_log_size, float signal_value, int signal_type)
#define DEFINE_FFT(name) \
  void Time ## name(int count, float signal_value, int signal_type)

#define DEFINE_FFT_ROUTINES(base) \
  DEFINE_FFT(base); \
  DEFINE_ONE_FFT(base)

#if defined(__arm__) || defined(__aarch64__)
DEFINE_FFT_ROUTINES(FloatFFT);
#endif

DEFINE_FFT_ROUTINES(FloatRFFT);

#ifdef ENABLE_FIXED_POINT_FFT_TESTS
DEFINE_FFT_ROUTINES(SC16FFT);
DEFINE_FFT_ROUTINES(SC32FFT);
DEFINE_FFT_ROUTINES(RFFT32);
/*
 * TimeOneRFFT16 has an extra arg to tell us what algorithm the RFFT16
 * method should use.  Perhaps this should be split out into two
 * different routines?
 */
DEFINE_FFT(RFFT16);
void TimeOneRFFT16(int count, int fft_log_size, float signal_value,
                   int signal_type, s16_s32 s16s32);
#endif

#if defined(HAVE_NE10)
/*
 * The Ne10 routines need to be initialized befer than can be used.
 * This routine must be called once to do that.
 */
void InitializeNE10();

DEFINE_FFT_ROUTINES(NE10FFT);
DEFINE_FFT_ROUTINES(NE10RFFT);
#endif

#if defined(HAVE_CKFFT)
DEFINE_FFT_ROUTINES(CkFFTFFT);
DEFINE_FFT_ROUTINES(CkFFTRFFT);
#endif

#if defined(HAVE_PFFFT)
DEFINE_FFT_ROUTINES(PfFFT);
DEFINE_FFT_ROUTINES(PfRFFT);
#endif

#if defined(HAVE_KISSFFT)
DEFINE_FFT_ROUTINES(KissFFT);
#endif

int verbose = 1;
int do_forward_test = 1;
int do_inverse_test = 1;
int min_fft_order = 2;
int max_fft_order = MAX_FFT_ORDER;
int include_conversion = 0;
static int adapt_count = 1;

void TimeFFTUsage(char* prog) {
  fprintf(stderr, 
      "%s: [-htTFICA] [-f fft] [-c count] [-n logsize] [-s scale]\n"
      "    [-g signal-type] [-S signal value]\n"
      "    [-m minFFTsize] [-M maxFFTsize]\n",
          ProgramName(prog));
  fprintf(stderr, 
#ifndef ARM_VFP_TEST
      "Simple FFT timing tests (NEON)\n"
#else
      "Simple FFT timing tests (non-NEON)\n"
#endif
      "  -h          This help\n"
      "  -v level    Verbose output level (default = 1)\n"
      "  -F          Skip forward FFT tests\n"
      "  -I          Skip inverse FFT tests\n"
      "  -C          Include float-to-fixed and fixed-to-float cost for\n"
      "              fixed-point FFTs.\n"
      "  -c count    Number of FFTs to compute for timing.  This is a\n"
      "              lower limit; shorter FFTs will do more FFTs such\n"
      "              that the elapsed time is very roughly constant, if\n"
      "              -A is not given.\n"
      "  -A          Don't adapt the count given by -c; use specified value\n"
      "  -m min      Mininum FFT order to test\n"
      "  -M max      Maximum FFT order to test\n"
      "  -t          Run timing test for all orders instead of just one\n"
      "                Only used if -T and -f are also given. -n is ignored in\n"
      "                this case.\n"
      "  -T          Run just one FFT timing test\n"
      "  -f          FFT type:\n"
      "              0 - Complex Float\n"
#if defined(__arm__) || defined(__aarch64__)
      "              1 - Real Float\n"
#endif
#ifdef ENABLE_FIXED_POINT_FFT_TESTS
      "              2 - Complex 16-bit\n"
      "              3 - Real 16-bit\n"
      "              4 - Complex 32-bit\n"
      "              5 - Real 32-bit\n"
#endif
#if defined(HAVE_KISSFFT)
      "              6 - KissFFT (complex)\n"
#endif
#if defined(HAVE_NE10)
      "              7 - NE10 complex float\n"
      "              8 - NE10 real float\n"
#endif
#if defined(HAVE_FFMPEG)
      "              9 - FFmpeg complex float\n"
      "              10 - FFmpeg real float\n"
#endif
#if defined(HAVE_CKFFT)
      "              11 - Cricket FFT complex float\n"
      "              12 - Cricket FFT real float\n"
#endif
#if defined(HAVE_PFFFT)
      "              13 - PFFFT complex float\n"
      "              14 - PFFFT real float\n"
#endif
      "  -n logsize  Log2 of FFT size\n"
      "  -s scale    Scale factor for forward FFT (default = 0)\n"
      "  -S signal   Base value for the test signal (default = 1024)\n"
      "  -g type     Input signal type:\n"
      "              0 - Constant signal S + i*S. (Default value.)\n"
      "              1 - Real ramp starting at S/N, N = FFT size\n"
      "              2 - Sine wave of amplitude S\n"
      "              3 - Complex signal whose transform is a sine wave.\n"
      "\n"
      "Use -v 0 in combination with -F or -I to get output that can\n"
      "be pasted into a spreadsheet.\n"
      "\n"
      "Most of the options listed after -T above are only applicable\n"
      "when -T is given to test just one FFT size and FFT type.\n"
      "\n");
  exit(0);
}

/* TODO(kma/ajm/rtoy): use strings instead of numbers for fft_type. */
int main(int argc, char* argv[]) {
  int fft_log_size = 4;
  float signal_value = 32767;
  int signal_type = 0;
  int test_mode = 1;
  int count = 100;
  int fft_type = 0;
  int fft_type_given = 0;
  int time_all_orders = 0;

  int opt;

  while ((opt = getopt(argc, argv, "htTFICAc:n:s:S:g:v:f:m:M:")) != -1) {
    switch (opt) {
      case 'h':
        TimeFFTUsage(argv[0]);
        break;
      case 't':
        time_all_orders = 1;
        break;
      case 'T':
        test_mode = 0;
        break;
      case 'C':
        include_conversion = 1;
        break;
      case 'F':
        do_forward_test = 0;
        break;
      case 'I':
        do_inverse_test = 0;
        break;
      case 'A':
        adapt_count = 0;
        break;
      case 'c':
        count = atoi(optarg);
        break;
      case 'n':
        fft_log_size = atoi(optarg);
        break;
      case 'S':
        signal_value = atof(optarg);
        break;
      case 'g':
        signal_type = atoi(optarg);
        break;
      case 'v':
        verbose = atoi(optarg);
        break;
      case 'f':
        fft_type = atoi(optarg);
        fft_type_given = 1;
        break;
      case 'm':
        min_fft_order = atoi(optarg);
        if (min_fft_order <= 2) {
          fprintf(stderr, "Setting min FFT order to 2 (from %d)\n",
                  min_fft_order);
          min_fft_order = 2;
        }
        break;
      case 'M':
        max_fft_order = atoi(optarg);
        if (max_fft_order > MAX_FFT_ORDER) {
          fprintf(stderr, "Setting max FFT order to %d (from %d)\n",
                  MAX_FFT_ORDER, max_fft_order);
          max_fft_order = MAX_FFT_ORDER;
        }
        break;
      default:
        TimeFFTUsage(argv[0]);
        break;
    }
  }

  if (test_mode && fft_type_given)
    printf("Warning:  -f ignored when -T not specified\n");

#if defined(HAVE_NE10)
  /* Need to initialize things if we have Ne10 available */
  InitializeNE10();
#endif

  if (test_mode) {
#if defined(__arm__) || defined(__aarch64__)
    TimeFloatFFT(count, signal_value, signal_type);
#endif
    TimeFloatRFFT(count, signal_value, signal_type);
#ifdef ENABLE_FIXED_POINT_FFT_TESTS
    TimeSC16FFT(count, signal_value, signal_type);
    TimeRFFT16(count, signal_value, signal_type);
    TimeSC32FFT(count, signal_value, signal_type);
    TimeRFFT32(count, signal_value, signal_type);
#endif
#if defined(HAVE_KISSFFT)
    TimeKissFFT(count, signal_value, signal_type);
#endif
#if defined(HAVE_NE10)
    TimeNE10FFT(count, signal_value, signal_type);
    TimeNE10RFFT(count, signal_value, signal_type);
#endif
#if defined(HAVE_FFMPEG)
    TimeFFmpegFFT(count, signal_value, signal_type);
    TimeFFmpegRFFT(count, signal_value, signal_type);
#endif
#if defined(HAVE_CKFFT)
    TimeCkFFTFFT(count, signal_value, signal_type);
    TimeCkFFTRFFT(count, signal_value, signal_type);
#endif
#if defined(HAVE_PFFFT)
    TimePfFFT(count, signal_value, signal_type);
    TimePfRFFT(count, signal_value, signal_type);
#endif
  } else {
    switch (fft_type) {
#if defined(__arm__) || defined(__aarch64__)
      case 0:
        if (time_all_orders)
          TimeFloatFFT(count, signal_value, signal_type);
        else
          TimeOneFloatFFT(count, fft_log_size, signal_value, signal_type);
        break;
#endif
      case 1:
        if (time_all_orders)
          TimeFloatRFFT(count, signal_value, signal_type);
        else
          TimeOneFloatRFFT(count, fft_log_size, signal_value, signal_type);
        break;
#ifdef ENABLE_FIXED_POINT_FFT_TESTS
      case 2:
        if (time_all_orders)
          TimeSC16FFT(count, signal_value, signal_type);
        else
          TimeOneSC16FFT(count, fft_log_size, signal_value, signal_type);
        break;
      case 3:
        if (time_all_orders) {
          TimeRFFT16(count, signal_value, signal_type);
        } else {
          TimeOneRFFT16(count, fft_log_size, signal_value, signal_type, S32);
          TimeOneRFFT16(count, fft_log_size, signal_value, signal_type, S16);
        }
        break;
      case 4:
        if (time_all_orders)
          TimeSC32FFT(count, signal_value, signal_type);
        else
          TimeOneSC32FFT(count, fft_log_size, signal_value, signal_type);
        break;
      case 5:
        if (time_all_orders)
          TimeRFFT32(count, signal_value, signal_type);
        else
          TimeOneRFFT32(count, fft_log_size, signal_value, signal_type);
        break;
#endif
#if defined(HAVE_KISSFFT)
      case 6:
        if (time_all_orders)
          TimeKissFFT(count, signal_value, signal_type);
        else
          TimeOneKissFFT(count, fft_log_size, signal_value, signal_type);
        break;
#endif
#if defined(HAVE_NE10)
      case 7:
        if (time_all_orders)
          TimeNE10FFT(count, signal_value, signal_type);
        else
          TimeOneNE10FFT(count, fft_log_size, signal_value, signal_type);
        break;
      case 8:
        if (time_all_orders)
          TimeNE10FFT(count, signal_value, signal_type);
        else
          TimeOneNE10RFFT(count, fft_log_size, signal_value, signal_type);
        break;
#endif
#if defined(HAVE_FFMPEG)
      case 9:
        if (time_all_orders)
          TimeFFmpegFFT(count, signal_value, signal_type);
        else
          TimeOneFFmpegFFT(count, fft_log_size, signal_value, signal_type);
        break;
      case 10:
        if (time_all_orders)
          TimeFFmpegRFFT(count, signal_value, signal_type);
        else
          TimeOneFFmpegRFFT(count, fft_log_size, signal_value, signal_type);
        break;
#endif
#if defined(HAVE_CKFFT)
      case 11:
        if (time_all_orders)
          TimeCkFFTFFT(count, signal_value, signal_type);
        else
          TimeOneCkFFTFFT(count, fft_log_size, signal_value, signal_type);
        break;
#endif
#if defined(HAVE_CKFFT)
      case 12:
        if (time_all_orders)
          TimeCkFFTRFFT(count, signal_value, signal_type);
        else
          TimeOneCkFFTRFFT(count, fft_log_size, signal_value, signal_type);
        break;
#endif
#if defined(HAVE_PFFFT)
      case 13:
        if (time_all_orders)
          TimePfFFT(count, signal_value, signal_type);
        else
          TimeOnePfFFT(count, fft_log_size, signal_value, signal_type);
        break;
      case 14:
        if (time_all_orders)
          TimePfRFFT(count, signal_value, signal_type);
        else
          TimeOnePfRFFT(count, fft_log_size, signal_value, signal_type);
        break;
#endif
      default:
        fprintf(stderr, "Unknown FFT type: %d\n", fft_type);
        break;
    }
  }

  return 0;
}

void GetUserTime(struct timeval* time) {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  memcpy(time, &usage.ru_utime, sizeof(*time));
}

double TimeDifference(const struct timeval * start,
                      const struct timeval * end) {
  double start_time;
  double end_time;
  start_time = start->tv_sec + start->tv_usec * 1e-6;
  end_time = end->tv_sec + end->tv_usec * 1e-6;

  return end_time - start_time;
}

void PrintShortHeader(const char* message) {
  if (do_forward_test && do_inverse_test) {
    /* Do nothing if both forward and inverse tests are being run. */
  } else if (do_forward_test) {
    printf("Forward ");
  } else {
    printf("Inverse ");
  }
  printf("%s\n", message);
}

void PrintResult(const char* prefix, int fft_log_size, double elapsed_time,
                 int count) {
  if (verbose == 0) {
    printf("%2d\t%8.4f\t%8d\t%.4e\n",
           fft_log_size, elapsed_time, count, 1000 * elapsed_time / count);
  } else {
    printf("%-18s:  order %2d:  %8.4f sec for %8d FFTs:  %.4e msec/FFT\n",
           prefix, fft_log_size, elapsed_time, count,
           1000 * elapsed_time / count);
  }
}

int ComputeCount(int nominal_count, int fft_log_size) {
  /*
   * Try to figure out how many repetitions to do for a given FFT
   * order (fft_log_size) given that we want a repetition of
   * nominal_count for order 15 FFTs to be the approsimate amount of
   * time we want to for all tests.
   */

  int count;
  if (adapt_count) {
    double maxTime = ((double) nominal_count) * (1 << MAX_FFT_ORDER)
        * MAX_FFT_ORDER;
    double c = maxTime / ((1 << fft_log_size) * fft_log_size);
    const int max_count = 10000000;

    count = (c > max_count) ? max_count : c;
  } else {
    count = nominal_count;
  }

  return count;
}

void GenerateRealFloatSignal(OMX_F32* x, void* fft_void, int size,
                             int signal_type, float signal_value)
{
  int k;
  struct ComplexFloat *test_signal;
  struct ComplexFloat *true_fft;

  OMX_FC32* fft = (OMX_FC32*) fft_void;
  
  test_signal = (struct ComplexFloat*) malloc(sizeof(*test_signal) * size);
  true_fft = (struct ComplexFloat*) malloc(sizeof(*true_fft) * size);
  GenerateTestSignalAndFFT(test_signal, true_fft, size, signal_type,
                           signal_value, 1);

  /*
   * Convert the complex result to what we want
   */

  for (k = 0; k < size; ++k) {
    x[k] = test_signal[k].Re;
  }

  for (k = 0; k < size / 2 + 1; ++k) {
    fft[k].Re = true_fft[k].Re;
    fft[k].Im = true_fft[k].Im;
  }

  free(test_signal);
  free(true_fft);
}

