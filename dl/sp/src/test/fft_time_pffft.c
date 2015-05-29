#if defined(HAVE_PFFFT)
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#if defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#endif

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"
#include "dl/sp/src/test/timing_util.h"

#include "../../../../third_party/other-fft/pffft/pffft.h"

/*
 * Scale FFT data by 1/|fftSize|. |length| is the length of the vector.
 */
static inline void ScaleVector(OMX_F32* vectorData, unsigned length, unsigned fftSize) {
#if defined(__arm__) || defined(__aarch64__)
  float32_t* data = (float32_t*)vectorData;
  float32_t scale = 1.0f / fftSize;

  if (length >= 4) {
    /*
     * Do 4 float elements at a time because |length| is always a
     * multiple of 4 when |length| >= 4.
     *
     * TODO(rtoy): Figure out how to process 8 elements at a time
     * using intrinsics or replace this with inline assembly.
     */
    do {
      float32x4_t x = vld1q_f32(data);

      length -= 4;
      x = vmulq_n_f32(x, scale);
      vst1q_f32(data, x);
      data += 4;
    } while (length > 0);
  } else if (length == 2) {
    float32x2_t x = vld1_f32(data);
    x = vmul_n_f32(x, scale);
    vst1_f32(data, x);
  } else {
    vectorData[0] *= scale;
  }
#else
  float scale = 1.0f / fftSize;
  for (m = 0; m < length; ++m) {
      vectorData[m] *= scale;
  }
#endif
}

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
    float scale = 1.0 / fft_size;

    memcpy(y, y_true, sizeof(*y) * (fft_size + 2));
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      int m;
      
      pffft_transform_ordered(s, (float*)y, (float*)z, NULL, PFFFT_BACKWARD);
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
  pffft_destroy_setup(s);
  free(y_true);
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

  y_true = (float*) malloc(sizeof(*y_true) * 2 * fft_size);
  y_tmp = (float*) malloc(sizeof(*y_tmp) * (fft_size + 2));

  x = x_aligned->aligned_pointer_;
  y = y_aligned->aligned_pointer_;
  z = z_aligned->aligned_pointer_;

  s = pffft_new_setup(fft_size, PFFFT_REAL);
  if (!s) {
    fprintf(stderr, "TimeOnePfFFT: Could not initialize structure for order %d\n",
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

    CompareComplexFloat(&snr_forward, (OMX_FC32*) y, (OMX_FC32*) y_true, fft_size / 2 + 1);

    PrintResult("Forward PFFFT FFT", fft_log_size, elapsed_time, count, snr_forward.complex_snr_);

    if (verbose >= 255) {
      printf("FFT Actual:\n");
      DumpArrayComplexFloat("y", fft_size / 2, (OMX_FC32*) y);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    float scale = 1.0 / fft_size;

    /* Copy y_true to true, but arrange the values according to what rdft wants. */

    memcpy(y_tmp, y_true, sizeof(y_tmp[0]) * fft_size);
    y_tmp[1] = y_true[fft_size / 2];

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      int m;
      
      pffft_transform_ordered(s, (float*)y_tmp, (float*)z, NULL, PFFFT_BACKWARD);
      /*
       * Need to include cost of scaling the inverse
       */
      ScaleVector(z, fft_size, fft_size);
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    CompareFloat(&snr_inverse, (OMX_F32*) z, (OMX_F32*) x, fft_size);

    PrintResult("Inverse PFFFT FFT", fft_log_size, elapsed_time, count, snr_inverse.complex_snr_);

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
  pffft_destroy_setup(s);
  free(y_true);
  free(y_tmp);
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
