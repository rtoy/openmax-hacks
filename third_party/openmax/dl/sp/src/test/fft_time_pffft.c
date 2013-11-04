#if defined(HAVE_PFFFT)
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"

#include "../other-fft/pffft/pffft.h"

extern int verbose;
extern int include_conversion;
extern int adapt_count;
extern int do_forward_test;
extern int do_inverse_test;
extern int min_fft_order;
extern int max_fft_order;

void TimeOnePfFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  struct ComplexFloat* x;
  struct ComplexFloat* y;
  OMX_FC32* z;

  struct ComplexFloat* y_true;

  int n;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  PFFFT_Setup *s;
  
  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  s = pffft_new_setup(fft_size, PFFFT_COMPLEX);
  if (!s) {
    fprintf(stderr, "TimeOnePfFFT: Could not initialize structure for order %d\n",
            fft_log_size);
  }

  GenerateTestSignalAndFFT(x, y_true, fft_size, signal_type, signal_value, 0);

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      pffft_transform_ordered(s, (float*)x, (float*)y, NULL, PFFFT_FORWARD);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    if (verbose > 255) {
      printf("FFT time          :  %g sec\n", elapsed_time);
    }

    if (verbose > 255)
      printf("Effective FFT time:  %g sec\n", elapsed_time);

    PrintResult("Forward PFFFT FFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    float scale = 1.0 / fft_size;

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      int m;
      
      pffft_transform_ordered(s, (float*)y, (float*)z, NULL, PFFFT_BACKWARD);
      /*
       * Need to include cost of scaling the inverse
       */
      for (m = 0; m < fft_size; ++m) {
        z[m].Re *= scale;
        z[m].Im *= scale;
      }
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    if (verbose > 255) {
      printf("FFT time          :  %g sec\n", elapsed_time);
    }

    if (verbose > 255)
      printf("Effective FFT time:  %g sec\n", elapsed_time);

    PrintResult("Inverse PFFFT FFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      printf("IFFT Actual:\n");
      DumpArrayComplexFloat("z", fft_size, z);
      printf("IFFT Expected:\n");
      DumpArrayComplexFloat("x", fft_size, (OMX_FC32*) x);
    }
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  pffft_destroy_setup(s);
  free(y_true);
}

void TimePfFFT(int count, float signal_value, int signal_type) {
  int k;
  int min_order;
  
  if (verbose == 0)
    printf("%s PFFFT FFT\n", do_forward_test ? "Forward" : "Inverse");

  /*
   * It appears that FFT orders below 4 are incorrect, so don't time
   * orders below 4.
   */
  min_order = min_fft_order < 4 ? 4 : min_fft_order;
  
  for (k = min_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOnePfFFT(testCount, k, signal_value, signal_type);
  }
}
#endif