#if defined(HAVE_CKFFT)
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

#include "../../../../third_party/other-fft/ckfft/inc/ckfft/ckfft.h"

void TimeOneCkFFTFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  struct ComplexFloat* x;
  struct ComplexFloat* y;
  OMX_FC32* z;

  struct ComplexFloat* y_true;

  int n;
  CkFftContext* fft_spec = NULL;;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  double copy_time = 0.0;
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

  GenerateTestSignalAndFFT(x, y_true, fft_size, signal_type, signal_value, 0);

  fft_spec = CkFftInit(fft_size, kCkFftDirection_Both, NULL, NULL);

  /*
   * Measure how much time we spend doing copies, so we can subtract
   * them from the elapsed time for the FFTs.
   */
  GetUserTime(&start_time);
  for (n = 0; n < count; ++n) {
    memcpy(z, y_true, sizeof(*z) * fft_size);
  }
  GetUserTime(&end_time);
  copy_time = TimeDifference(&start_time, &end_time);

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(y, x, sizeof(*y) * fft_size);
      CkFftComplexForward(fft_spec, fft_size, (CkFftComplex*) x, (CkFftComplex*) y);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    if (verbose > 255) {
      printf("FFT time          :  %g sec\n", elapsed_time);
      printf("Time for copying  :  %g sec\n", copy_time);
    }

    elapsed_time -= copy_time;

    if (verbose > 255)
      printf("Effective FFT time:  %g sec\n", elapsed_time);

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size);

    PrintResult("Forward CkFFT FFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(z, y_true, sizeof(*z) * fft_size);
      CkFftComplexInverse(fft_spec, fft_size, (CkFftComplex*) y_true, (CkFftComplex*) z);

      // CkFftComplexInverse doesn't scale the inverse by 1/N, so we
      // need to do it since the other FFTs do.
      ScaleVector((OMX_F32*) z, 2 * fft_size, fft_size);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_inverse, (OMX_FC32*) z, (OMX_FC32*) x, fft_size);

    if (verbose > 255) {
      printf("FFT time          :  %g sec\n", elapsed_time);
      printf("Time for copying  :  %g sec\n", copy_time);
    }

    elapsed_time -= copy_time;

    if (verbose > 255)
      printf("Effective FFT time:  %g sec\n", elapsed_time);

    PrintResult("Inverse CkFFT FFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

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
  free(y_true);
  CkFftShutdown(fft_spec);
}

void TimeCkFFTFFT(int count, float signal_value, int signal_type) {
  int k;

  if (verbose == 0)
    printf("%s CkFFT FFT\n", do_forward_test ? "Forward" : "Inverse");
  
  for (k = min_fft_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneCkFFTFFT(testCount, k, signal_value, signal_type);
  }
}

void TimeOneCkFFTRFFT(int count, int fft_log_size, float signal_value,
                      int signal_type) {
  OMX_F32* x;                   /* Source */
  OMX_F32* y;                   /* Transform */
  OMX_F32* z;                   /* Inverse transform */

  OMX_F32* y_true;              /* True FFT */
  OMX_F32* tmp;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;


  int n;
  CkFftContext* fft_spec = NULL;;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  struct SnrResult snr_forward;
  struct SnrResult snr_inverse;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  /* The transformed value is in CCS format and is has fft_size + 2 values */
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  y_true = (OMX_F32*) malloc(sizeof(*y_true) * (fft_size + 2));
  tmp = (OMX_F32*) malloc(sizeof(*tmp) * (fft_size + 2));

  GenerateRealFloatSignal(x, (struct ComplexFloat*) y_true, fft_size, signal_type,
                          signal_value);

  fft_spec = CkFftInit(fft_size, kCkFftDirection_Both, NULL, NULL);

  if (!fft_spec) {
    fprintf(stderr, "TimeOneCkFFTRFFT:  Could not initialize structures for order %d\n",
            fft_log_size);
    return;
  }

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      CkFftRealForward(fft_spec, fft_size, x, (CkFftComplex*)y);
      ScaleVector(y, fft_size + 2, 2);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size / 2 + 1);
    
    if (verbose > 255) {
      printf("FFT time          :  %g sec\n", elapsed_time);
    }

    PrintResult("Forward CkFFT RFFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

    if (verbose >= 255) {
      OMX_FC32* fft = (OMX_FC32*) y;
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size / 2 + 1, fft);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      CkFftRealInverse(fft_spec, fft_size, (CkFftComplex*)y_true, z, (CkFftComplex*) tmp);

      // CkFftComplexInverse doesn't scale the inverse by 1/N, so we
      // need to do it since the other FFTs do.
      ScaleVector(z, fft_size, fft_size);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareFloat(&snr_inverse, (OMX_F32*) z, (OMX_F32*) x, fft_size);

    if (verbose > 255) {
      printf("IFFT time          :  %g sec\n", elapsed_time);
    }

    PrintResult("Inverse CkFFT RFFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

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
  free(tmp);
  CkFftShutdown(fft_spec);
}

void TimeCkFFTRFFT(int count, float signal_value, int signal_type) {
  int k;

  if (verbose == 0)
    printf("%s CkFFT RFFT\n", do_forward_test ? "Forward" : "Inverse");
  
  for (k = min_fft_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneCkFFTRFFT(testCount, k, signal_value, signal_type);
  }
}
#endif
