#if defined(HAVE_PFFFT)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"
#include "dl/sp/src/test/timing_util.h"

#include "../../../../third_party/other-fft/pffft/pffft.h"

void TimeOnePfFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;
  struct AlignedPtr* y_true_aligned;

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
  struct SnrResult snr_forward;
  struct SnrResult snr_inverse;
  
  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);
  y_true_aligned = AllocAlignedPointer(32, sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;
  y_true = y_true_aligned->aligned_pointer_;

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

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size);

    PrintResult("Forward PFFFT FFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    memcpy(y, y_true, sizeof(*y) * (fft_size + 2));
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      pffft_transform_ordered(s, (float*)y_true, (float*)z, NULL, PFFFT_BACKWARD);
      /*
       * Need to include cost of scaling the inverse
       */
      ScaleVector((OMX_F32*) z, 2 * fft_size, fft_size);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_inverse, (OMX_FC32*) z, (OMX_FC32*) x, fft_size);

    PrintResult("Inverse PFFFT FFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

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
  FreeAlignedPointer(y_true_aligned);
  pffft_destroy_setup(s);
}

void TimePfFFT(int count, float signal_value, int signal_type) {
  int k;
  int min_order;
  
  if (verbose == 0)
    printf("%s PFFFT FFT\n", do_forward_test ? "Forward" : "Inverse");

  /*
   * Orders less than 4 are not supported by PFFFT.
   */
  min_order = min_fft_order < 4 ? 4 : min_fft_order;
  
  for (k = min_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOnePfFFT(testCount, k, signal_value, signal_type);
  }
}

void TimeOnePfRFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;
  struct AlignedPtr* y_tmp_aligned;

  float* x;
  struct ComplexFloat* y;
  OMX_F32* z;

  float* y_true;
  float* y_tmp;

  int n;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  PFFFT_Setup *s;
  struct SnrResult snr_forward;
  struct SnrResult snr_inverse;
  
  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);
  y_tmp_aligned = AllocAlignedPointer(32, sizeof(*y_tmp) * (fft_size + 2));

  y_true = (float*) malloc(sizeof(*y_true) * 2 * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;
  y_tmp = y_tmp_aligned->aligned_pointer_;

  s = pffft_new_setup(fft_size, PFFFT_REAL);
  if (!s) {
    fprintf(stderr, "TimeOnePfRFFT: Could not initialize structure for order %d\n",
            fft_log_size);
  }

  GenerateRealFloatSignal(x, (struct ComplexFloat*) y_true, fft_size, signal_type, signal_value);

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      pffft_transform_ordered(s, (float*)x, (float*)y, NULL, PFFFT_FORWARD);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    /*
     * Arrange the output of the FFT to match the expected output.
     */
    y[fft_size / 2].Re = y[0].Im;
    y[fft_size / 2].Im = 0;
    y[0].Im = 0;

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size / 2 + 1);

    PrintResult("Forward PFFFT RFFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size / 2 + 1, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    /* Copy y_true to true, but arrange the values according to what rdft wants. */

    memcpy(y_tmp, y_true, sizeof(y_tmp[0]) * fft_size);
    y_tmp[1] = y_true[fft_size / 2];

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      pffft_transform_ordered(s, (float*)y_tmp, (float*)z, NULL, PFFFT_BACKWARD);
      /*
       * Need to include cost of scaling the inverse
       */
      ScaleVector(z, fft_size, fft_size);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareFloat(&snr_inverse, (OMX_F32*) z, (OMX_F32*) x, fft_size);

    PrintResult("Inverse PFFFT RFFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

    if (verbose >= 255) {
      printf("IFFT Actual:\n");
      DumpArrayFloat("z", fft_size, z);
      printf("IFFT Expected:\n");
      DumpArrayFloat("x", fft_size, x);
    }
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  FreeAlignedPointer(y_tmp_aligned);
  pffft_destroy_setup(s);
  free(y_true);
}

void TimePfRFFT(int count, float signal_value, int signal_type) {
  int k;
  int min_order;
  
  if (verbose == 0)
    printf("%s PFFFT RFFT\n", do_forward_test ? "Forward" : "Inverse");

  /*
   * Orders less than 8 are not supported by PFFFT.
   */
  min_order = min_fft_order < 5 ? 5 : min_fft_order;
  
  for (k = min_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOnePfRFFT(testCount, k, signal_value, signal_type);
  }
}

#endif
