#if defined(HAVE_KISSFFT)
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"
#include "dl/sp/src/test/timing_util.h"

#include "../../../../third_party/other-fft/kiss_fft130/kiss_fft.h"
#include "../../../../third_party/other-fft/kiss_fft130/tools/kiss_fftr.h"

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
  struct SnrResult snr_forward;
  struct SnrResult snr_inverse;

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

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size);

    PrintResult("Forward Kiss FFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

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

    CompareComplexFloat(&snr_inverse, (OMX_FC32*) z, (OMX_FC32*) x, fft_size);

    PrintResult("Inverse Kiss FFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

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

  kiss_fft_scalar* x;
  kiss_fft_cpx* y;
  kiss_fft_cpx* z;

  struct ComplexFloat* y_true;

  int n;
  kiss_fftr_cfg fft_fwd_spec;
  kiss_fftr_cfg fft_inv_spec;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  struct SnrResult snr_forward;
  struct SnrResult snr_inverse;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  GenerateRealFloatSignal(x, (struct ComplexFloat*) y_true, fft_size, signal_type, signal_value);

  fft_fwd_spec = kiss_fftr_alloc(fft_size, 0, 0, 0);
  fft_inv_spec = kiss_fftr_alloc(fft_size, 1, 0, 0);

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      kiss_fftr(fft_fwd_spec, x, y);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size / 2 + 1);

    PrintResult("Forward Kiss FFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size / 2 + 1, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
  }

#if 0
  if (do_inverse_test) {
    double scale = 1.0 / fft_size;
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      int k;
      kiss_fftr(fft_inv_spec, (kiss_fft_cpx*) y_true, z);

      // kiss_fft does not scale the inverse transform so do it here.
      
      for (k = 0; k < fft_size; ++k) {
        z[k].r *= scale;
        z[k].i *= scale;
      }
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_inverse, (OMX_FC32*) z, (OMX_FC32*) x, fft_size);

    PrintResult("Inverse Kiss FFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

    if (verbose >= 255) {
      printf("IFFT Actual:\n");
      DumpArrayComplexFloat("z", fft_size, (OMX_FC32*) z);
      printf("IFFT Expected:\n");
      DumpArrayComplexFloat("x", fft_size, (OMX_FC32*) x);
      printf("%4s\t%10s.re[n]\t%10s.im[n]\n", "n", "z", "z");
    }
  }
#endif

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(y_true);
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeKissRFFT(int count, float signal_value, int signal_type) {
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
    TimeOneKissRFFT(testCount, k, signal_value, signal_type);
  }
}
#endif
