#if defined(HAVE_NE10)
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"
#include "../other-fft/Ne10/inc/NE10_types.h"

extern int verbose;
extern int include_conversion;
extern int adapt_count;
extern int do_forward_test;
extern int do_inverse_test;
extern int min_fft_order;
extern int max_fft_order;

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
  ne10_cfft_radix4_instance_f32_t fft_fwd_spec;
  ne10_cfft_radix4_instance_f32_t fft_inv_spec ;
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;

  fft_size = 1 << fft_log_size;

  if (!((fft_size == 16) || (fft_size == 64) || (fft_size == 256) || (fft_size == 1024))) {
    fprintf(stderr, "NE10 FFT: invalid FFT size: %d\n", fft_size);
    return;
  }
  x_aligned = AllocAlignedPointer(32, sizeof(*x) * fft_size);
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * fft_size);

  y_true = (struct ComplexFloat*) malloc(sizeof(*y_true) * fft_size);

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  GenerateTestSignalAndFFT(x, y_true, fft_size, signal_type, signal_value, 0);

  status = ne10_cfft_radix4_init_float(&fft_fwd_spec, fft_size, 0);
  if (status == NE10_ERR) {
    fprintf(stderr, "NE10 FFT: Cannot initialize FFT structure for order %d\n", fft_log_size);
    return;
  }
  ne10_cfft_radix4_init_float(&fft_inv_spec, fft_size, 1);
  
  if (do_forward_test) {
    // Ne10 FFTs destroy the input.
    struct ComplexFloat *saved_x =
        (struct ComplexFloat*) malloc(fft_size * sizeof(struct ComplexFloat*));
    
    memcpy(saved_x, x, fft_size * sizeof(struct ComplexFloat*));

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(x, saved_x, fft_size * sizeof(struct ComplexFloat*));
      ne10_radix4_butterfly_float_neon(y, x, fft_size, fft_fwd_spec.p_twiddle);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResult("Forward NE10 FFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size, (OMX_FC32*) y_true);
    }

    free(saved_x);
  }

  if (do_inverse_test) {
    // Ne10 FFTs destroy the input.
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(y, y_true, fft_size * sizeof(*y));

      // The inverse doesn't appear to be working.  Or I'm calling it
      // incorrectly.
      ne10_radix4_butterfly_inverse_float_neon(z,
                                               y,
                                               fft_size,
                                               fft_inv_spec.p_twiddle,
                                               fft_inv_spec.one_by_fft_len);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResult("Inverse NE10 FFT", fft_log_size, elapsed_time, count);
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
}

void TimeNE10FFT(int count, float signal_value, int signal_type) {
  int k;

  if (verbose == 0)
    printf("%s NE10 FFT\n", do_forward_test ? "Forward" : "Inverse");
  
  // Currently, NE10 only supports sizes 16, 64, 256, and 1024 (Order 4, 6, 8, 10).
  for (k = 4; k <= 10; k += 2) {
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
  struct AlignedPtr* t_aligned;


  int n;
  ne10_result_t status;
  ne10_cfft_radix4_instance_f32_t fft_fwd_spec;
  ne10_cfft_radix4_instance_f32_t fft_inv_spec ;
  ne10_rfft_instance_f32_t rfft_fwd_spec;
  ne10_rfft_instance_f32_t rfft_inv_spec;
  
  int fft_size;
  struct timeval start_time;
  struct timeval end_time;
  double elapsed_time;

  fft_size = 1 << fft_log_size;

  if (!((fft_size == 128) || (fft_size == 512) || (fft_size == 2048))) {
    fprintf(stderr, "NE10 RFFT: invalid FFT size: %d\n", fft_size);
    return;
  }

  x_aligned = AllocAlignedPointer(32, sizeof(*x) * 2 * fft_size);
  /* The transformed value is in CCS format and is has fft_size + 2 values */
  y_aligned = AllocAlignedPointer(32, sizeof(*y) * (2 * fft_size + 2));
  z_aligned = AllocAlignedPointer(32, sizeof(*z) * 2 * fft_size);
  t_aligned = AllocAlignedPointer(32, sizeof(*temp) * (2 * fft_size + 2));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;
  temp = t_aligned->aligned_pointer_;

  y_true = (OMX_F32*) malloc(sizeof(*y_true) * (fft_size + 2));

  GenerateRealFloatSignal(x, (struct ComplexFloat*) y_true, fft_size, signal_type,
                          signal_value);

  status = ne10_rfft_init_float(&rfft_fwd_spec, &fft_fwd_spec, fft_size, 0);

  if (status == NE10_ERR) {
    fprintf(stderr, "NE10 RFFT: Cannot initialize FFT structure for order %d\n", fft_log_size);
    return;
  }
  ne10_rfft_init_float(&rfft_inv_spec, &fft_inv_spec, fft_size, 1);

  if (do_forward_test) {
    // Ne10 FFTs destroy the input.
    float *saved_x = (float*) malloc(fft_size * sizeof(*saved_x));

    memcpy(saved_x, x, fft_size * sizeof(*saved_x));
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(x, saved_x, fft_size * sizeof(*saved_x));
      ne10_rfft_float_neon(&rfft_fwd_spec, x, y, temp);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResult("Forward NE10 RFFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size / 2 + 1, y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
    free(saved_x);
  }

  if (do_inverse_test) {
    // Ne10 FFTs destroy the input.

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(y, y_true, (fft_size >> 1) * sizeof(*y));
      // The inverse appears not to be working.
      ne10_rfft_float_neon(&rfft_inv_spec, y, z, temp);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    PrintResult("Inverse NE10 RFFT", fft_log_size, elapsed_time, count);
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
}

void TimeNE10RFFT(int count, float signal_value, int signal_type) {
  int k;

  if (verbose == 0)
    printf("%s NE10 RFFT\n", do_forward_test ? "Forward" : "Inverse");
  
  // The NE10 RFFT routine currently only supports sizes 128, 512, 2048. (Order 7, 9, 11)
  for (k = 7; k <= 11; k += 2) {
    int testCount = ComputeCount(count, k);
    TimeOneNE10RFFT(testCount, k, signal_value, signal_type);
  }
}

#endif
