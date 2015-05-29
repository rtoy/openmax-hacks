#ifndef TIMING_UTIL_H_
#define TIMING_UTIL_H_

#if defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#endif

#define ENABLE_FIXED_POINT_FFT_TESTS

#if defined(FLOAT_ONLY) || defined(ARM_VFP_TEST)
/*
 * Fixed-point FFTs are disabled if we only want float tests or if
 * we're building for non-NEON tests.
 */
#undef ENABLE_FIXED_POINT_FFT_TESTS
#endif

#define MAX_FFT_ORDER_FIXED_POINT 12

/*
 * Enum for kind of routine to use when timing one RFFT16 FFT.
 */
typedef enum {
  S16,
  S32,
} s16_s32;

/*
 * The different kinds of FFTs supported by this timing program. 
 */

enum {
  OPENMAX_COMPLEX_FLOAT,
  OPENMAX_REAL_FLOAT,
  OPENMAX_COMPLEX_16BIT,
  OPENMAX_REAL_16BIT,
  OPENMAX_COMPLEX_32BIT,
  OPENMAX_REAL_32BIT,
  KISSFFT_COMPLEX_FLOAT = 10,
  NE10_COMPLEX_FLOAT = 12,
  NE10_REAL_FLOAT,
  FFMPEG_COMPLEX_FLOAT = 14,
  FFMPEG_REAL_FLOAT,
  CRICKET_COMPLEX_FLOAT = 16,
  CRICKET_REAL_FLOAT,
  PFFFT_COMPLEX_FLOAT = 18,
  PFFFT_REAL_FLOAT
};

extern int verbose;
extern int include_conversion;
extern int do_forward_test;
extern int do_inverse_test;
extern int min_fft_order;
extern int max_fft_order;

void GetUserTime(struct timeval* time);
double TimeDifference(const struct timeval * start,
                      const struct timeval * end);
void PrintResult(const char* prefix, int fft_log_size, double elapsed_time,
                 int count, double snr);
void PrintResultNoSNR(const char* prefix, int fft_log_size, double elapsed_time,
                      int count);
void PrintShortHeader(const char* message);
int ComputeCount(int nominal_count, int fft_log_size);
void GenerateRealFloatSignal(OMX_F32* x, void* fft, int size,
                             int signal_type, float signal_value);
/*
 * Multiply each element fo a float vector of length |length| by the
 * value 1 / |fftSize|.
 *
 * This can be used to perform the scaling needed for an inverse FFT
 * by using the appropriate values for |length| and |fftSize|.  In
 * particular, for a complex float vector of length |n|, set |length|
 * = 2*|n| and set |fftSize| = |n|. 
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

#endif
