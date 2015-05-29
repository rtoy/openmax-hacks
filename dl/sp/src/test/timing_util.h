#ifndef TIMING_UTIL_H_
#define TIMING_UTIL_H_

#define ENABLE_FIXED_POINT_FFT_TESTS

#if defined(FLOAT_ONLY) || defined(ARM_VFP_TEST)
/*
 * Fixed-point FFTs are disabled if we only want float tests or if
 * we're building for non-NEON tests.
 */
#undef ENABLE_FIXED_POINT_FFT_TESTS
#endif

#define MAX_FFT_ORDER_FIXED_POINT 12

typedef enum {
  S16,
  S32,
} s16_s32;

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
                 int count);
void PrintShortHeader(const char* message);
int ComputeCount(int nominal_count, int fft_log_size);
void GenerateRealFloatSignal(OMX_F32* x, void* fft, int size,
                             int signal_type, float signal_value);
#endif
