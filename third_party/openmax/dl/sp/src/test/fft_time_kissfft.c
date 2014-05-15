#if defined(HAVE_KISSFFT)
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"

#include "../other-fft/kiss_fft130/kiss_fft.h"
#include "../other-fft/kiss_fft130/tools/kiss_fftr.h"

extern int verbose;
extern int include_conversion;
extern int adapt_count;
extern int do_forward_test;
extern int do_inverse_test;
extern int min_fft_order;
extern int max_fft_order;

void TimeOneKissFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  kiss_fft_cpx* x;
  kiss_fft_cpx* y;
  kiss_fft_cpx* z;

  struct ComplexFloat* y_true;

  int n;
  kiss_fft_cfg fft_fwd_spec;
  kiss_fft_cfg fft_inv_spec;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  GenerateTestSignalAndFFT((struct ComplexFloat*) x, y_true, fft_size, signal_type, signal_value, 0);

  fft_fwd_spec = kiss_fft_alloc(fft_size, 0, 0, 0);
  fft_inv_spec = kiss_fft_alloc(fft_size, 1, 0, 0);

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      kiss_fft(fft_fwd_spec, x, y);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResult("Forward Kiss FFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    double scale = 1.0 / fft_size;
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      int k;
      kiss_fft(fft_inv_spec, (kiss_fft_cpx*) y_true, z);

      // kiss_fft does not scale the inverse transform so do it here.
      
      for (k = 0; k < fft_size; ++k) {
        z[k].r *= scale;
        z[k].i *= scale;
      }
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResult("Inverse Kiss FFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      printf("IFFT Actual:\n");
      DumpArrayComplexFloat("z", fft_size, (OMX_FC32*) z);
      printf("IFFT Expected:\n");
      DumpArrayComplexFloat("x", fft_size, (OMX_FC32*) x);
      printf("%4s\t%10s.re[n]\t%10s.im[n]\n", "n", "z", "z");
    }
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(y_true);
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeKissFFT(int count, float signal_value, int signal_type) {
  int k;

  if (verbose == 0)
    printf("%s Kiss FFT\n", do_forward_test ? "Forward" : "Inverse");
  
  for (k = min_fft_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneKissFFT(testCount, k, signal_value, signal_type);
  }
}

void TimeOneKissRFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  float* x;
  struct ComplexFloat* y;
  OMX_F32* z;

  float* y_true;
  float* true;

  int n;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  kiss_fftr_cfg fft_fwd_spec;
  kiss_fftr_cfg fft_inv_spec;
  
  
  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  y_true = (float*) malloc(sizeof(*y_true) * 2 * fft_size);
  true = (float*) malloc(sizeof(*true) * (fft_size + 2));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  fft_fwd_spec = kiss_fftr_alloc(fft_size, 0, 0, 0);
  fft_inv_spec = kiss_fftr_alloc(fft_size, 1, 0, 0);
  if (!fft_fwd_spec) {
    fprintf(stderr, "TimeOneKissFFT: Could not initialize forward structure for order %d\n",
            fft_log_size);
  }
  if (!fft_inv_spec) {
    fprintf(stderr, "TimeOneKissFFT: Could not initialize inverse structure for order %d\n",
            fft_log_size);
  }

  GenerateRealFloatSignal(x, (struct ComplexFloat*) y_true, fft_size, signal_type, signal_value);

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      kiss_fftr(fft_fwd_spec, x, (kiss_fft_cpx*) y);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    if (verbose > 255) {
      printf("FFT time          :  %g sec\n", elapsed_time);
    }

    if (verbose > 255)
      printf("Effective FFT time:  %g sec\n", elapsed_time);

    PrintResult("Forward KISS RFFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size / 2 + 1, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    float scale = 1.0 / fft_size;

    /* Copy y_true to true, but arrange the values according to what rdft wants. */

    memcpy(true, y_true, sizeof(float*) * fft_size);
    true[1] = y_true[fft_size / 2];

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      int m;
      
      kiss_fftri(fft_inv_spec, (kiss_fft_cpx*) y, (kiss_fft_scalar*) z);
      /*
       * Need to include cost of scaling the inverse
       */
      for (m = 0; m < fft_size; ++m) {
        z[m] *= scale;
      }
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    if (verbose > 255) {
      printf("FFT time          :  %g sec\n", elapsed_time);
    }

    PrintResult("Inverse KISS RFFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      printf("IFFT Actual:\n");
      DumpArrayFloat("z", fft_size, z);
      printf("IFFT Expected:\n");
      DumpArrayFloat("x", fft_size, (OMX_F32*) x);
    }
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(y_true);
  free(true);
}

void TimeKissRFFT(int count, float signal_value, int signal_type) {
  int k;
  
  if (verbose == 0)
    printf("%s Kiss RFFT\n", do_forward_test ? "Forward" : "Inverse");

  for (k = min_fft_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneKissRFFT(testCount, k, signal_value, signal_type);
  }
}
#endif
