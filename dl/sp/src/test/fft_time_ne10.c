#if defined(HAVE_NE10)
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"
#include "dl/sp/src/test/timing_util.h"

#include "../../../../third_party/other-fft/Ne10/inc/NE10_types.h"
#include "../../../../third_party/other-fft/Ne10/inc/NE10_dsp.h"

void InitializeNE10()
{
  /* Need to initialize things if we have Ne10 available */
  extern int omxSP_HasArmNeon();
  extern ne10_result_t ne10_init_dsp (ne10_int32_t is_NEON_available);
  ne10_init_dsp(omxSP_HasArmNeon());
}

void TimeOneNE10FFT(int count, int fft_log_size, float signal_value,
                    int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  struct ComplexFloat* x;
  struct ComplexFloat* y;
  OMX_FC32* z;

  struct ComplexFloat* y_true;

  int n;
  ne10_result_t status;
  ne10_fft_cfg_float32_t fft_fwd_spec;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  struct SnrResult snr_forward;
  struct SnrResult snr_inverse;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * 2 * fft_size);
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * 2 * fft_size);

  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  GenerateTestSignalAndFFT(x, y_true, fft_size, signal_type, signal_value, 0);

  fft_fwd_spec = ne10_fft_alloc_c2c_float32(fft_size);
  if (!fft_fwd_spec) {
    fprintf(stderr, "NE10 FFT: Cannot initialize FFT structure for order %d\n", fft_log_size);
    return;
  }
  
  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      ne10_fft_c2c_1d_float32_neon((ne10_fft_cpx_float32_t *) y,
                                   (ne10_fft_cpx_float32_t *) x,
                                   fft_fwd_spec,
                                   0);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size);

    PrintResult("Forward NE10 FFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

    if (verbose >= 255) {
      printf("Input data:\n");
      DumpArrayComplexFloat("x", fft_size, (OMX_FC32*) x);
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      ne10_fft_c2c_1d_float32_neon((ne10_fft_cpx_float32_t *) z,
                                   (ne10_fft_cpx_float32_t *) y_true,
                                   fft_fwd_spec,
                                   1);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_inverse, (OMX_FC32*) z, (OMX_FC32*) x, fft_size);

    PrintResult("Inverse NE10 FFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

    if (verbose >= 255) {
      printf("Input data:\n");
      DumpArrayComplexFloat("y", fft_size, (OMX_FC32*) y_true);
      printf("IFFT Actual:\n");
      DumpArrayComplexFloat("z", fft_size, z);
      printf("IFFT Expected:\n");
      DumpArrayComplexFloat("x", fft_size, (OMX_FC32*) x);
    }
  }

  free(fft_fwd_spec);
  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
  free(y_true);
}

void TimeNE10FFT(int count, float signal_value, int signal_type) {
  int k;
  int min_order = min_fft_order >= 2 ? min_fft_order : 2;

  if (verbose == 0)
    printf("%s NE10 FFT\n", do_forward_test ? "Forward" : "Inverse");
  
  // Currently, NE10 only supports sizes 16, 64, 256, and 1024 (Order 4, 6, 8, 10).
  for (k = min_fft_order; k <= max_fft_order; k++) {
    int testCount = ComputeCount(count, k);
    TimeOneNE10FFT(testCount, k, signal_value, signal_type);
  }
}

void TimeOneNE10RFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  OMX_F32* x;                   /* Source */
  OMX_FC32* y;                  /* Transform */
  OMX_F32* z;                   /* Inverse transform */
  OMX_F32* temp;

  OMX_F32* y_true;              /* True FFT */

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  int n;
  ne10_result_t status;
  ne10_fft_r2c_cfg_float32_t fft_fwd_spec;
  
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;
  struct SnrResult snr_forward;
  struct SnrResult snr_inverse;

  fft_size = 1 << fft_log_size;

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * 4 * fft_size);
  /* The transformed value is in CCS format and is has fft_size + 2 values */
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (4 * fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * 4 * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  y_true = (OMX_F32*) malloc(sizeof(*y_true) * (fft_size + 2));

  GenerateRealFloatSignal(x, (struct ComplexFloat*) y_true, fft_size, signal_type,
                          signal_value);

  fft_fwd_spec = ne10_fft_alloc_r2c_float32(fft_size);

  if (!fft_fwd_spec) {
    fprintf(stderr, "NE10 RFFT: Cannot initialize FFT structure for order %d\n", fft_log_size);
    return;
  }

  if (do_forward_test) {
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      ne10_fft_r2c_1d_float32_neon((ne10_fft_cpx_float32_t *) y,
                                   x,
                                   fft_fwd_spec);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size / 2 + 1);
    
    PrintResult("Forward NE10 RFFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size / 2 + 1, y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    // Ne10 FFTs destroy the input.

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      //memcpy(y, y_true, (fft_size >> 1) * sizeof(*y));
      // The inverse appears not to be working.
      ne10_fft_c2r_1d_float32_neon(z,
                                   (ne10_fft_cpx_float32_t *) y_true,
                                   fft_fwd_spec);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareFloat(&snr_inverse, (OMX_F32*) z, (OMX_F32*) x, fft_size);

    PrintResult("Inverse NE10 RFFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

    if (verbose >= 255) {
      printf("IFFT Actual:\n");
      DumpArrayFloat("z", fft_size, z);
      printf("IFFT Expected:\n");
      DumpArrayFloat("x", fft_size, x);
    }
  }

  free(fft_fwd_spec);
  FreeAlignedPointer(x_aligned);
  FreeAlignedPointer(y_aligned);
  FreeAlignedPointer(z_aligned);
}

void TimeNE10RFFT(int count, float signal_value, int signal_type) {
  int k;
  int min_order = min_fft_order >= 3 ? min_fft_order : 3;

  if (verbose == 0)
    printf("%s NE10 RFFT\n", do_forward_test ? "Forward" : "Inverse");
  
  // The NE10 RFFT routine currently only supports sizes 128, 512, 2048. (Order 7, 9, 11)
  for (k = min_order; k <= max_fft_order; k++) {
    int testCount = ComputeCount(count, k);
    TimeOneNE10RFFT(testCount, k, signal_value, signal_type);
  }
}

#endif
