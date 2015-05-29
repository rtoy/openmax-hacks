#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"
#include "dl/sp/src/test/timing_util.h"

#if defined(ARM_VFP_TEST)
#define FORWARD_FLOAT_FFT   omxSP_FFTFwd_CToC_FC32_Sfs_vfp
#define INVERSE_FLOAT_FFT   omxSP_FFTInv_CToC_FC32_Sfs_vfp
#define FORWARD_FLOAT_RFFT  omxSP_FFTFwd_RToCCS_F32_Sfs_vfp
#define INVERSE_FLOAT_RFFT  omxSP_FFTInv_CCSToR_F32_Sfs_vfp
#else
#define FORWARD_FLOAT_FFT   omxSP_FFTFwd_CToC_FC32_Sfs
#define INVERSE_FLOAT_FFT   omxSP_FFTInv_CToC_FC32_Sfs
#define FORWARD_FLOAT_RFFT  omxSP_FFTFwd_RToCCS_F32_Sfs
#define INVERSE_FLOAT_RFFT  omxSP_FFTInv_CCSToR_F32_Sfs
#endif


#if defined(__arm__) || defined(__aarch64__)
void TimeOneFloatFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  struct ComplexFloat* x;
  struct ComplexFloat* y;
  OMX_FC32* z;

  struct ComplexFloat* y_true;

  OMX_INT n, fft_spec_buffer_size;
  OMXFFTSpec_C_FC32 * fft_fwd_spec = NULL;
  OMXFFTSpec_C_FC32 * fft_inv_spec = NULL;
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

  GenerateTestSignalAndFFT(x, y_true, fft_size, signal_type, signal_value, 0);

  omxSP_FFTGetBufSize_C_FC32(fft_log_size, &fft_spec_buffer_size);

  fft_fwd_spec = (OMXFFTSpec_C_FC32*) malloc(fft_spec_buffer_size);
  fft_inv_spec = (OMXFFTSpec_C_FC32*) malloc(fft_spec_buffer_size);
  omxSP_FFTInit_C_FC32(fft_fwd_spec, fft_log_size);
  omxSP_FFTInit_C_FC32(fft_inv_spec, fft_log_size);

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      FORWARD_FLOAT_FFT((OMX_FC32*) x, (OMX_FC32*) y, fft_fwd_spec);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size);

    PrintResult("Forward Float FFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);
  }

  if (do_inverse_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      INVERSE_FLOAT_FFT((OMX_FC32*) y, z, fft_inv_spec);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_inverse, (OMX_FC32*) z, (OMX_FC32*) x, fft_size);

    PrintResult("Inverse Float FFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(y_true);
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeFloatFFT(int count, float signal_value, int signal_type) {
  int k;

  if (verbose == 0)
    PrintShortHeader("Float FFT");

  for (k = min_fft_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneFloatFFT(testCount, k, signal_value, signal_type);
  }
}
#endif

void TimeOneFloatRFFT(int count, int fft_log_size, float signal_value,
                      int signal_type) {
  OMX_F32* x;                   /* Source */
  OMX_F32* y;                   /* Transform */
  OMX_F32* z;                   /* Inverse transform */

  OMX_F32* y_true;              /* True FFT */

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;


  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_R_F32 * fft_fwd_spec = NULL;
  OMXFFTSpec_R_F32 * fft_inv_spec = NULL;
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

  GenerateRealFloatSignal(x, (OMX_FC32*) y_true, fft_size, signal_type,
                          signal_value);

  status = omxSP_FFTGetBufSize_R_F32(fft_log_size, &fft_spec_buffer_size);

  fft_fwd_spec = (OMXFFTSpec_R_F32*) malloc(fft_spec_buffer_size);
  fft_inv_spec = (OMXFFTSpec_R_F32*) malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_R_F32(fft_fwd_spec, fft_log_size);

  status = omxSP_FFTInit_R_F32(fft_inv_spec, fft_log_size);

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      FORWARD_FLOAT_RFFT(x, y, fft_fwd_spec);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size / 2 + 1);

    PrintResult("Forward Float RFFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);
  }

  if (do_inverse_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      INVERSE_FLOAT_RFFT(y, z, fft_inv_spec);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareFloat(&snr_inverse, (OMX_F32*) z, (OMX_F32*) x, fft_size);

    PrintResult("Inverse Float RFFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeFloatRFFT(int count, float signal_value, int signal_type) {
  int k;

  if (verbose == 0)
    PrintShortHeader("Float RFFT");
  
  for (k = min_fft_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneFloatRFFT(testCount, k, signal_value, signal_type);
  }
}

#ifdef ENABLE_FIXED_POINT_FFT_TESTS
void generateSC32Signal(OMX_SC32* x, OMX_SC32* fft, int size, int signal_type,
                        float signal_value) {
  int k;
  struct ComplexFloat *test_signal;
  struct ComplexFloat *true_fft;

  test_signal = (struct ComplexFloat*) malloc(sizeof(*test_signal) * size);
  true_fft = (struct ComplexFloat*) malloc(sizeof(*true_fft) * size);
  GenerateTestSignalAndFFT(test_signal, true_fft, size, signal_type,
                           signal_value, 0);

  /*
   * Convert the complex result to what we want
   */

  for (k = 0; k < size; ++k) {
    x[k].Re = 0.5 + test_signal[k].Re;
    x[k].Im = 0.5 + test_signal[k].Im;
    fft[k].Re = 0.5 + true_fft[k].Re;
    fft[k].Im = 0.5 + true_fft[k].Im;
  }

  free(test_signal);
  free(true_fft);
}

void TimeOneSC32FFT(int count, int fft_log_size, float signal_value,
                    int signal_type) {
  OMX_SC32* x;
  OMX_SC32* y;
  OMX_SC32* z;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  OMX_SC32* y_true;
  OMX_SC32* temp32a;
  OMX_SC32* temp32b;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_C_SC32 * fft_fwd_spec = NULL;
  OMXFFTSpec_C_SC32 * fft_inv_spec = NULL;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * fft_size);
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);
  y_true = (OMX_SC32*) malloc(sizeof(*y_true) * fft_size);
  temp32a = (OMX_SC32*) malloc(sizeof(*temp32a) * fft_size);
  temp32b = (OMX_SC32*) malloc(sizeof(*temp32b) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  generateSC32Signal(x, y_true, fft_size, signal_type, signal_value);

  status = omxSP_FFTGetBufSize_C_SC32(fft_log_size, &fft_spec_buffer_size);

  fft_fwd_spec = (OMXFFTSpec_C_SC32*) malloc(fft_spec_buffer_size);
  fft_inv_spec = (OMXFFTSpec_C_SC32*) malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_C_SC32(fft_fwd_spec, fft_log_size);

  status = omxSP_FFTInit_C_SC32(fft_inv_spec, fft_log_size);

  if (do_forward_test) {
    if (include_conversion) {
      int k;
      float factor = -1;

      GetUserTime(&start_time);
      for (k = 0; k < count; ++k) {
        for (n = 0; n < fft_size; ++n) {
          if (abs(x[n].Re) > factor) {
            factor = abs(x[n].Re);
          }
          if (abs(x[n].Im) > factor) {
            factor = abs(x[n].Im);
          }
        }

        factor = ((1 << 18) - 1) / factor;
        for (n = 0; n < fft_size; ++n) {
          temp32a[n].Re = factor * x[n].Re;
          temp32a[n].Im = factor * x[n].Im;
        }

        omxSP_FFTFwd_CToC_SC32_Sfs(x, y, fft_fwd_spec, 0);

        factor = 1 / factor;
        for (n = 0; n < fft_size; ++n) {
          temp32b[n].Re = y[n].Re * factor;
          temp32b[n].Im = y[n].Im * factor;
        }
      }
      GetUserTime(&end_time);
    } else {
      GetUserTime(&start_time);
      for (n = 0; n < count; ++n) {
        omxSP_FFTFwd_CToC_SC32_Sfs(x, y, fft_fwd_spec, 0);
      }
      GetUserTime(&end_time);
    }

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResultNoSNR("Forward SC32 FFT", fft_log_size, elapsed_time, count);
  }

  if (do_inverse_test) {
    if (include_conversion) {
      int k;
      float factor = -1;

      GetUserTime(&start_time);
      for (k = 0; k < count; ++k) {
        for (n = 0; n < fft_size; ++n) {
          if (abs(x[n].Re) > factor) {
            factor = abs(x[n].Re);
          }
          if (abs(x[n].Im) > factor) {
            factor = abs(x[n].Im);
          }
        }
        factor = ((1 << 18) - 1) / factor;
        for (n = 0; n < fft_size; ++n) {
          temp32a[n].Re = factor * x[n].Re;
          temp32a[n].Im = factor * x[n].Im;
        }

        status = omxSP_FFTInv_CToC_SC32_Sfs(y, z, fft_inv_spec, 0);

        factor = 1 / factor;
        for (n = 0; n < fft_size; ++n) {
          temp32b[n].Re = y[n].Re * factor;
          temp32b[n].Im = y[n].Im * factor;
        }
      }
      GetUserTime(&end_time);
    } else {
      GetUserTime(&start_time);
      for (n = 0; n < count; ++n) {
        status = omxSP_FFTInv_CToC_SC32_Sfs(y, z, fft_inv_spec, 0);
      }
      GetUserTime(&end_time);
    }

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResultNoSNR("Inverse SC32 FFT", fft_log_size, elapsed_time, count);
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(temp32a);
  free(temp32b);
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeSC32FFT(int count, float signal_value, int signal_type) {
  int k;
  int max_order = (max_fft_order > MAX_FFT_ORDER_FIXED_POINT)
      ? MAX_FFT_ORDER_FIXED_POINT : max_fft_order;

  if (verbose == 0)
    PrintShortHeader("SC32 FFT");

  for (k = min_fft_order; k <= max_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneSC32FFT(testCount, k, signal_value, signal_type);
  }
}

void generateSC16Signal(OMX_SC16* x, OMX_SC16* fft, int size, int signal_type,
                        float signal_value) {
  int k;
  struct ComplexFloat *test_signal;
  struct ComplexFloat *true_fft;

  test_signal = (struct ComplexFloat*) malloc(sizeof(*test_signal) * size);
  true_fft = (struct ComplexFloat*) malloc(sizeof(*true_fft) * size);
  GenerateTestSignalAndFFT(test_signal, true_fft, size, signal_type,
                           signal_value, 0);

  /*
   * Convert the complex result to what we want
   */

  for (k = 0; k < size; ++k) {
    x[k].Re = 0.5 + test_signal[k].Re;
    x[k].Im = 0.5 + test_signal[k].Im;
    fft[k].Re = 0.5 + true_fft[k].Re;
    fft[k].Im = 0.5 + true_fft[k].Im;
  }

  free(test_signal);
  free(true_fft);
}

void TimeOneSC16FFT(int count, int fft_log_size, float signal_value,
                    int signal_type) {
  OMX_SC16* x;
  OMX_SC16* y;
  OMX_SC16* z;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  OMX_SC16* y_true;
  OMX_SC16* temp16a;
  OMX_SC16* temp16b;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_C_SC16 * fft_fwd_spec = NULL;
  OMXFFTSpec_C_SC16 * fft_inv_spec = NULL;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * fft_size);
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);
  y_true = (OMX_SC16*) malloc(sizeof(*y_true) * fft_size);
  temp16a = (OMX_SC16*) malloc(sizeof(*temp16a) * fft_size);
  temp16b = (OMX_SC16*) malloc(sizeof(*temp16b) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  generateSC16Signal(x, y_true, fft_size, signal_type, signal_value);

  status = omxSP_FFTGetBufSize_C_SC16(fft_log_size, &fft_spec_buffer_size);

  fft_fwd_spec = (OMXFFTSpec_C_SC16*) malloc(fft_spec_buffer_size);
  fft_inv_spec = (OMXFFTSpec_C_SC16*) malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_C_SC16(fft_fwd_spec, fft_log_size);

  status = omxSP_FFTInit_C_SC16(fft_inv_spec, fft_log_size);

  if (do_forward_test) {
    if (include_conversion) {
      int k;
      float factor = -1;

      GetUserTime(&start_time);
      for (k = 0; k < count; ++k) {
        for (n = 0; n < fft_size; ++n) {
          if (abs(x[n].Re) > factor) {
            factor = abs(x[n].Re);
          }
          if (abs(x[n].Im) > factor) {
            factor = abs(x[n].Im);
          }
        }

        factor = ((1 << 15) - 1) / factor;
        for (n = 0; n < fft_size; ++n) {
          temp16a[n].Re = factor * x[n].Re;
          temp16a[n].Im = factor * x[n].Im;
        }

        omxSP_FFTFwd_CToC_SC16_Sfs(x, y, fft_fwd_spec, 0);

        factor = 1 / factor;
        for (n = 0; n < fft_size; ++n) {
          temp16b[n].Re = y[n].Re * factor;
          temp16b[n].Im = y[n].Im * factor;
        }
      }
      GetUserTime(&end_time);
    } else {
      GetUserTime(&start_time);
      for (n = 0; n < count; ++n) {
        omxSP_FFTFwd_CToC_SC16_Sfs(x, y, fft_fwd_spec, 0);
      }
      GetUserTime(&end_time);
    }

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResultNoSNR("Forward SC16 FFT", fft_log_size, elapsed_time, count);
  }

  if (do_inverse_test) {
    if (include_conversion) {
      int k;
      float factor = -1;

      GetUserTime(&start_time);
      for (k = 0; k < count; ++k) {
        for (n = 0; n < fft_size; ++n) {
          if (abs(x[n].Re) > factor) {
            factor = abs(x[n].Re);
          }
          if (abs(x[n].Im) > factor) {
            factor = abs(x[n].Im);
          }
        }
        factor = ((1 << 15) - 1) / factor;
        for (n = 0; n < fft_size; ++n) {
          temp16a[n].Re = factor * x[n].Re;
          temp16a[n].Im = factor * x[n].Im;
        }

        status = omxSP_FFTInv_CToC_SC16_Sfs(y, z, fft_inv_spec, 0);

        factor = 1 / factor;
        for (n = 0; n < fft_size; ++n) {
          temp16b[n].Re = y[n].Re * factor;
          temp16b[n].Im = y[n].Im * factor;
        }
      }
      GetUserTime(&end_time);
    } else {
      GetUserTime(&start_time);
      for (n = 0; n < count; ++n) {
        status = omxSP_FFTInv_CToC_SC16_Sfs(y, z, fft_inv_spec, 0);
      }
      GetUserTime(&end_time);
    }

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResultNoSNR("Inverse SC16 FFT", fft_log_size, elapsed_time, count);
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(temp16a);
  free(temp16b);
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeSC16FFT(int count, float signal_value, int signal_type) {
  int k;
  int max_order = (max_fft_order > MAX_FFT_ORDER_FIXED_POINT)
      ? MAX_FFT_ORDER_FIXED_POINT : max_fft_order;

  if (verbose == 0)
    PrintShortHeader("SC16 FFT");

  for (k = min_fft_order; k <= max_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneSC16FFT(testCount, k, signal_value, signal_type);
  }
}

void GenerateRFFT16Signal(OMX_S16* x, OMX_SC32* fft, int size, int signal_type,
                          float signal_value) {
  int k;
  struct ComplexFloat *test_signal;
  struct ComplexFloat *true_fft;

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

/* Argument s16s32:
 *     S32:       Calculate RFFT16 with 32 bit complex FFT;
 *     otherwise: Calculate RFFT16 with 16 bit complex FFT.
 */
void TimeOneRFFT16(int count, int fft_log_size, float signal_value,
                   int signal_type, s16_s32 s16s32) {
  OMX_S16* x;
  OMX_S32* y;
  OMX_S16* z;
  OMX_S32* y_true;
  OMX_F32* xr;
  OMX_F32* yrTrue;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;
  struct AlignedPtr* y_trueAligned;
  struct AlignedPtr* xr_aligned;
  struct AlignedPtr* yr_true_aligned;


  OMX_S16* temp16;
  OMX_S32* temp32;


  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_R_S16 * fft_fwd_spec = NULL;
  OMXFFTSpec_R_S16 * fft_inv_spec = NULL;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  int scaleFactor;

  fft_size = 1 << fft_log_size;
  scaleFactor = fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  y_trueAligned = AllocAlignedPointer(32, sizeof(*y_true) * (fft_size + 2));

  xr_aligned = AllocAlignedPointer(32, sizeof(*xr) * fft_size);
  yr_true_aligned = AllocAlignedPointer(32, sizeof(*yrTrue) * (fft_size + 2));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;
  y_true = y_trueAligned->aligned_pointer_;
  xr = xr_aligned->aligned_pointer_;
  yrTrue = yr_true_aligned->aligned_pointer_;

  temp16 = (OMX_S16*) malloc(sizeof(*temp16) * fft_size);
  temp32 = (OMX_S32*) malloc(sizeof(*temp32) * fft_size);


  GenerateRFFT16Signal(x, (OMX_SC32*) y_true, fft_size, signal_type,
                       signal_value);
  /*
   * Generate a real version so we can measure scaling costs
   */
  GenerateRealFloatSignal(xr, (OMX_FC32*) yrTrue, fft_size, signal_type,
                          signal_value);

  if(s16s32 == S32) {
    status = omxSP_FFTGetBufSize_R_S16S32(fft_log_size, &fft_spec_buffer_size);
    fft_fwd_spec = malloc(fft_spec_buffer_size);
    fft_inv_spec = malloc(fft_spec_buffer_size);
    status = omxSP_FFTInit_R_S16S32(fft_fwd_spec, fft_log_size);
    status = omxSP_FFTInit_R_S16S32(fft_inv_spec, fft_log_size);
  }
  else {
    status = omxSP_FFTGetBufSize_R_S16(fft_log_size, &fft_spec_buffer_size);
    fft_fwd_spec = malloc(fft_spec_buffer_size);
    fft_inv_spec = malloc(fft_spec_buffer_size);
    status = omxSP_FFTInit_R_S16(fft_fwd_spec, fft_log_size);
    status = omxSP_FFTInit_R_S16(fft_inv_spec, fft_log_size);
  }

  if (do_forward_test) {
    if (include_conversion) {
      int k;
      float factor = -1;

      GetUserTime(&start_time);
      for (k = 0; k < count; ++k) {
        /*
         * Spend some time computing the max of the signal, and then scaling it.
         */
        for (n = 0; n < fft_size; ++n) {
          if (abs(xr[n]) > factor) {
            factor = abs(xr[n]);
          }
        }

        factor = 32767 / factor;
        for (n = 0; n < fft_size; ++n) {
          temp16[n] = factor * xr[n];
        }

        if(s16s32 == S32) {
          status = omxSP_FFTFwd_RToCCS_S16S32_Sfs(x, y,
              (OMXFFTSpec_R_S16S32*)fft_fwd_spec, (OMX_INT) scaleFactor);
        }
        else {
          status = omxSP_FFTFwd_RToCCS_S16_Sfs(x,  (OMX_S16*)y,
              (OMXFFTSpec_R_S16*)fft_fwd_spec, (OMX_INT) scaleFactor);
        }
        /*
         * Now spend some time converting the fixed-point FFT back to float.
         */
        factor = 1 / factor;
        for (n = 0; n < fft_size + 2; ++n) {
          xr[n] = y[n] * factor;
        }
      }
      GetUserTime(&end_time);
    } else {
      float factor = -1;

      GetUserTime(&start_time);
      for (n = 0; n < count; ++n) {
      if(s16s32 == S32) {
        status = omxSP_FFTFwd_RToCCS_S16S32_Sfs(x, y, 
            (OMXFFTSpec_R_S16S32*)fft_fwd_spec, (OMX_INT) scaleFactor);
      }
      else {
        status = omxSP_FFTFwd_RToCCS_S16_Sfs(x, (OMX_S16*)y,
            (OMXFFTSpec_R_S16*)fft_fwd_spec, (OMX_INT) scaleFactor);
      }
      }
      GetUserTime(&end_time);
    }

    elapsed_time = TimeDifference(&start_time, &end_time);

    if(s16s32 == S32) {
      PrintResultNoSNR("Forward RFFT16 (with S32)",
                       fft_log_size, elapsed_time, count);
    }
    else {
      PrintResultNoSNR("Forward RFFT16 (with S16)",
                       fft_log_size, elapsed_time, count);
    }
  }

  if (do_inverse_test) {
    if (include_conversion) {
      int k;
      float factor = -1;

      GetUserTime(&start_time);
      for (k = 0; k < count; ++k) {
        /*
         * Spend some time scaling the FFT signal to fixed point.
         */
        for (n = 0; n < fft_size; ++n) {
          if (abs(yrTrue[n]) > factor) {
            factor = abs(yrTrue[n]);
          }
        }
        for (n = 0; n < fft_size; ++n) {
          temp32[n] = factor * yrTrue[n];
        }

        if(s16s32 == S32) {
          status = omxSP_FFTInv_CCSToR_S32S16_Sfs(y, z,
              (OMXFFTSpec_R_S16S32*)fft_inv_spec, 0);
        }
        else {
          status = omxSP_FFTInv_CCSToR_S16_Sfs((OMX_S16*)y, z,
              (OMXFFTSpec_R_S16*)fft_inv_spec, 0);
        }
        /*
         * Spend some time converting the result back to float
         */
        factor = 1 / factor;
        for (n = 0; n < fft_size; ++n) {
          xr[n] = factor * z[n];
        }
      }
      GetUserTime(&end_time);
    } else {
      GetUserTime(&start_time);
      for (n = 0; n < count; ++n) {
        if(s16s32 == S32) {
          status = omxSP_FFTInv_CCSToR_S32S16_Sfs(y, z,
              (OMXFFTSpec_R_S16S32*)fft_inv_spec, 0);
        }
        else {
          status = omxSP_FFTInv_CCSToR_S16_Sfs((OMX_S16*)y, z,
              (OMXFFTSpec_R_S16*)fft_inv_spec, 0);
        }
      }
      GetUserTime(&end_time);
    }

    elapsed_time = TimeDifference(&start_time, &end_time);

    if(s16s32 == S32) {
      PrintResultNoSNR("Inverse RFFT16 (with S32)",
                       fft_log_size, elapsed_time, count);
    }
    else {
      PrintResultNoSNR("Inverse RFFT16 (with S16)",
                       fft_log_size, elapsed_time, count);
    }
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  FreeAlignedPointer(y_trueAligned);
  FreeAlignedPointer(xr_aligned);
  FreeAlignedPointer(yr_true_aligned);
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeRFFT16(int count, float signal_value, int signal_type) {
  int k;
  int max_order = (max_fft_order > MAX_FFT_ORDER_FIXED_POINT)
      ? MAX_FFT_ORDER_FIXED_POINT : max_fft_order;

  if (verbose == 0)
    PrintShortHeader("RFFT16 (with S32)");

  for (k = min_fft_order; k <= max_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneRFFT16(testCount, k, signal_value, signal_type, 1);
  }

  if (verbose == 0)
    PrintShortHeader("RFFT16 (with S16)");

  for (k = min_fft_order; k <= max_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneRFFT16(testCount, k, signal_value, signal_type, 0);
  }
}

void GenerateRFFT32Signal(OMX_S32* x, OMX_SC32* fft, int size, int signal_type,
                          float signal_value) {
  int k;
  struct ComplexFloat *test_signal;
  struct ComplexFloat *true_fft;

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

void TimeOneRFFT32(int count, int fft_log_size, float signal_value,
                   int signal_type) {
  OMX_S32* x;
  OMX_S32* y;
  OMX_S32* z;
  OMX_S32* y_true;
  OMX_F32* xr;
  OMX_F32* yrTrue;

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;
  struct AlignedPtr* y_true_aligned;

  OMX_S32* temp1;
  OMX_S32* temp2;

  OMX_INT n, fft_spec_buffer_size;
  OMXResult status;
  OMXFFTSpec_R_S16S32 * fft_fwd_spec = NULL;
  OMXFFTSpec_R_S16S32 * fft_inv_spec = NULL;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  int scaleFactor;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  y_true_aligned = AllocAlignedPointer(32, sizeof(*y_true) * (fft_size + 2));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;
  y_true = y_true_aligned->aligned_pointer_;

  if (verbose > 3) {
    printf("x = %p\n", (void*)x);
    printf("y = %p\n", (void*)y);
    printf("z = %p\n", (void*)z);
  }

  xr = (OMX_F32*) malloc(sizeof(*x) * fft_size);
  yrTrue = (OMX_F32*) malloc(sizeof(*y) * (fft_size + 2));
  temp1 = (OMX_S32*) malloc(sizeof(*temp1) * fft_size);
  temp2 = (OMX_S32*) malloc(sizeof(*temp2) * (fft_size + 2));

  GenerateRFFT32Signal(x, (OMX_SC32*) y_true, fft_size, signal_type,
                       signal_value);

  if (verbose > 63) {
    printf("Signal\n");
    printf("n\tx[n]\n");
    for (n = 0; n < fft_size; ++n) {
      printf("%4d\t%d\n", n, x[n]);
    }
  }

  status = omxSP_FFTGetBufSize_R_S32(fft_log_size, &fft_spec_buffer_size);
  if (verbose > 3) {
    printf("fft_spec_buffer_size = %d\n", fft_spec_buffer_size);
  }

  fft_fwd_spec = (OMXFFTSpec_R_S32*) malloc(fft_spec_buffer_size);
  fft_inv_spec = (OMXFFTSpec_R_S32*) malloc(fft_spec_buffer_size);
  status = omxSP_FFTInit_R_S32(fft_fwd_spec, fft_log_size);
  if (status) {
    printf("Failed to init forward FFT:  status = %d\n", status);
  }

  status = omxSP_FFTInit_R_S32(fft_inv_spec, fft_log_size);
  if (status) {
    printf("Failed to init backward FFT:  status = %d\n", status);
  }

  if (do_forward_test) {
    if (include_conversion) {
      int k;
      float factor = -1;

      GetUserTime(&start_time);
      for (k = 0; k < count; ++k) {
        /*
         * Spend some time computing the max of the signal, and then scaling it.
         */
        for (n = 0; n < fft_size; ++n) {
          if (abs(xr[n]) > factor) {
            factor = abs(xr[n]);
          }
        }

        factor = (1 << 20) / factor;
        for (n = 0; n < fft_size; ++n) {
          temp1[n] = factor * xr[n];
        }

        status = omxSP_FFTFwd_RToCCS_S32_Sfs(x, y, fft_fwd_spec,
                                             (OMX_INT) scaleFactor);

        /*
         * Now spend some time converting the fixed-point FFT back to float.
         */
        factor = 1 / factor;
        for (n = 0; n < fft_size + 2; ++n) {
          xr[n] = y[n] * factor;
        }
      }
      GetUserTime(&end_time);
    } else {
      float factor = -1;

      GetUserTime(&start_time);
      for (n = 0; n < count; ++n) {
        status = omxSP_FFTFwd_RToCCS_S32_Sfs(x, y, fft_fwd_spec,
                                             (OMX_INT) scaleFactor);
      }
      GetUserTime(&end_time);
    }

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResultNoSNR("Forward RFFT32", fft_log_size, elapsed_time, count);
  }

  if (do_inverse_test) {
    if (include_conversion) {
      int k;
      float factor = -1;

      GetUserTime(&start_time);
      for (k = 0; k < count; ++k) {
        /*
         * Spend some time scaling the FFT signal to fixed point.
         */
        for (n = 0; n < fft_size + 2; ++n) {
          if (abs(yrTrue[n]) > factor) {
            factor = abs(yrTrue[n]);
          }
        }
        for (n = 0; n < fft_size + 2; ++n) {
          temp2[n] = factor * yrTrue[n];
        }

        status = omxSP_FFTInv_CCSToR_S32_Sfs(y, z, fft_inv_spec, 0);

        /*
         * Spend some time converting the result back to float
         */
        factor = 1 / factor;
        for (n = 0; n < fft_size; ++n) {
          xr[n] = factor * z[n];
        }
      }
      GetUserTime(&end_time);
    } else {
      GetUserTime(&start_time);
      for (n = 0; n < count; ++n) {
        status = omxSP_FFTInv_CCSToR_S32_Sfs(y, z, fft_inv_spec, 0);
      }
      GetUserTime(&end_time);
    }

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResultNoSNR("Inverse RFFT32", fft_log_size, elapsed_time, count);
  }

  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  FreeAlignedPointer(y_true_aligned);
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeRFFT32(int count, float signal_value, int signal_type) {
  int k;
  int max_order = (max_fft_order > MAX_FFT_ORDER_FIXED_POINT)
      ? MAX_FFT_ORDER_FIXED_POINT : max_fft_order;

  if (verbose == 0)
    PrintShortHeader("RFFT32");

  for (k = min_fft_order; k <= max_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneRFFT32(testCount, k, signal_value, signal_type);
  }
}
#endif
