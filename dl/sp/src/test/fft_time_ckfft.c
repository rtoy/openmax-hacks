#if defined(HAVE_CKFFT)
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"

#include "../../../../third_party/other-fft/ckfft/inc/ckfft/ckfft.h"

extern int verbose;
extern int include_conversion;
extern int adapt_count;
extern int do_forward_test;
extern int do_inverse_test;
extern int min_fft_order;
extern int max_fft_order;
void GetUserTime(struct timeval* time);
double TimeDifference(const struct timeval * start,
                      const struct timeval * end);
void PrintResult(const char* prefix, int fft_log_size, double elapsed_time,
                 int count);
int ComputeCount(int nominal_count, int fft_log_size);
void GenerateRealFloatSignal(OMX_F32* x, void* fft, int size,
                             int signal_type, float signal_value);

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
  CkFftContext* fft_fwd_spec = NULL;;
  CkFftContext* fft_inv_spec = NULL;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  double copy_time = 0.0;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  GenerateTestSignalAndFFT(x, y_true, fft_size, signal_type, signal_value, 0);

  fft_fwd_spec = CkFftInit(fft_size, kCkFftDirection_Forward, NULL, NULL);
  fft_inv_spec = CkFftInit(fft_size, kCkFftDirection_Inverse, NULL, NULL);

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
      CkFftComplexForward(fft_fwd_spec, fft_size, (CkFftComplex*) x, (CkFftComplex*) y);
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

    PrintResult("Forward CkFFT FFT", fft_log_size, elapsed_time, count);
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
      
      memcpy(z, y_true, sizeof(*z) * fft_size);
      CkFftComplexInverse(fft_inv_spec, fft_size, (CkFftComplex*) y, (CkFftComplex*) z);

      // CkFftComplexInverse doesn't scale the inverse by 1/N, so we
      // need to do it since the other FFTs do.
      for (m = 0; m < fft_size; ++m) {
        z[m].Re *= scale;
        z[m].Im *= scale;
      }
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

    PrintResult("Inverse CkFFT FFT", fft_log_size, elapsed_time, count);
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
  free(fft_fwd_spec);
  free(fft_inv_spec);
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
  CkFftContext* fft_fwd_spec = NULL;;
  CkFftContext* fft_inv_spec = NULL;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  double copy_time = 0.0;

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

  fft_fwd_spec = CkFftInit(fft_size, kCkFftDirection_Forward, NULL, NULL);
  fft_inv_spec = CkFftInit(fft_size, kCkFftDirection_Inverse, NULL, NULL);

  if (!fft_fwd_spec || !fft_inv_spec) {
    fprintf(stderr, "TimeOneCkFFTRFFT:  Could not initialize structures for order %d\n",
            fft_log_size);
    return;
  }

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      CkFftRealForward(fft_fwd_spec, fft_size, x, (CkFftComplex*)y);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    if (verbose > 255) {
      printf("FFT time          :  %g sec\n", elapsed_time);
    }

    PrintResult("Forward CkFFT RFFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      OMX_FC32* fft = (OMX_FC32*) y;
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size / 2, fft);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    float scale = 2.0 / fft_size;

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      int m;
      
      CkFftRealInverse(fft_inv_spec, fft_size, (CkFftComplex*)y, z, (CkFftComplex*) tmp);

      // CkFftComplexInverse doesn't scale the inverse by 1/N, so we
      // need to do it since the other FFTs do.

      for (m = 0; m < fft_size; ++m) {
        z[m] *= scale;
      }
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    if (verbose > 255) {
      printf("IFFT time          :  %g sec\n", elapsed_time);
    }

    PrintResult("Inverse CkFFT RFFT", fft_log_size, elapsed_time, count);
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
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeCkFFTRFFT(int count, float signal_value, int signal_type) {
  int k;
  int min_order;

  if (verbose == 0)
    printf("%s CkFFT RFFT\n", do_forward_test ? "Forward" : "Inverse");
  
  for (k = min_fft_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneCkFFTRFFT(testCount, k, signal_value, signal_type);
  }
}
#endif
