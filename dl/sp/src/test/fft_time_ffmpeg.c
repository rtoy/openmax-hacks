#if defined(HAVE_FFMPEG)
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"
#include "dl/sp/src/test/aligned_ptr.h"
#include "dl/sp/src/test/gensig.h"
#include "dl/sp/src/test/test_util.h"

#include "../other-fft/ffmpeg/libavcodec/avfft.h"

extern int verbose;
extern int include_conversion;
extern int adapt_count;
extern int do_forward_test;
extern int do_inverse_test;
extern int min_fft_order;
extern int max_fft_order;

void TimeOneFFmpegFFT(int count, int fft_log_size, float signal_value,
                     int signal_type) {
  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;

  struct ComplexFloat* x;
  struct ComplexFloat* y;
  OMX_FC32* z;

  struct ComplexFloat* y_true;

  int n;
  FFTContext* fft_fwd_spec = NULL;;
  FFTContext* fft_inv_spec = NULL;
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

  fft_fwd_spec = av_fft_init(fft_log_size, 0);
  fft_inv_spec = av_fft_init(fft_log_size, 1);

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
      av_fft_permute(fft_fwd_spec, (FFTComplex*) y);
      av_fft_calc(fft_fwd_spec, (FFTComplex*) y);
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

    PrintResultNoSNR("Forward FFmpeg FFT", fft_log_size, elapsed_time, count);
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
      av_fft_permute(fft_inv_spec, (FFTComplex*) z);
      av_fft_calc(fft_inv_spec, (FFTComplex*) z);

      // av_fft_calc doesn't scale the inverse by 1/N, so we need to
      // do it since the other FFTs do.
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

    PrintResultNoSNR("Inverse FFmpeg FFT", fft_log_size, elapsed_time, count);
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

void TimeFFmpegFFT(int count, float signal_value, int signal_type) {
  int k;

  if (verbose == 0)
    printf("%s FFmpeg FFT\n", do_forward_test ? "Forward" : "Inverse");
  
  for (k = min_fft_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneFFmpegFFT(testCount, k, signal_value, signal_type);
  }
}

void TimeOneFFmpegRFFT(int count, int fft_log_size, float signal_value,
                      int signal_type) {
  OMX_F32* x;                   /* Source */
  OMX_F32* y;                   /* Transform */
  OMX_F32* z;                   /* Inverse transform */

  OMX_F32* y_true;              /* True FFT */

  struct AlignedPtr* x_aligned;
  struct AlignedPtr* y_aligned;
  struct AlignedPtr* z_aligned;


  int n;
  RDFTContext* fft_fwd_spec;
  RDFTContext* fft_inv_spec;
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

  GenerateRealFloatSignal(x, (struct ComplexFloat*) y_true, fft_size, signal_type,
                          signal_value);

  fft_fwd_spec = av_rdft_init(fft_log_size, DFT_R2C);
  fft_inv_spec = av_rdft_init(fft_log_size, IDFT_C2R);

  if (!fft_fwd_spec || !fft_inv_spec) {
    fprintf(stderr, "TimeOneFFmpegRFFT:  Could not initialize structures for order %d\n",
            fft_log_size);
    return;
  }

  if (do_forward_test) {
    /*
     * Measure how much time we spend doing copies, so we can subtract
     * them from the elapsed time for the FFTs.
     */
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(y, x, sizeof(*y) * fft_size);
    }
    GetUserTime(&end_time);
    copy_time = TimeDifference(&start_time, &end_time);

    
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(y, x, sizeof(*y) * fft_size);
      av_rdft_calc(fft_fwd_spec, (FFTSample*) y);
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

    PrintResultNoSNR("Forward FFmpeg RFFT", fft_log_size, elapsed_time, count);
    if (verbose >= 255) {
      OMX_FC32* fft = (OMX_FC32*) y;
      printf("FFT Actual (FFMPEG packed format):\n");
      DumpArrayComplexFloat("y", fft_size / 2, fft);
      printf("FFT Expected:\n");
      DumpArrayComplexFloat("true", fft_size / 2 + 1, (OMX_FC32*) y_true);
    }
  }

  if (do_inverse_test) {
    float scale = 2.0 / fft_size;
    FFTSample* true = (FFTSample*) malloc(sizeof(*true) * (fft_size + 2));

    /* Copy y_true to true, but arrange the values according to what rdft wants. */

    memcpy(true, y_true, sizeof(FFTSample*) * fft_size);
    true[1] = y_true[fft_size / 2];

    /*
     * Measure how much time we spend doing copies, so we can subtract
     * them from the elapsed time for the FFTs.
     */
    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      memcpy(z, true, sizeof(*z) * fft_size);
    }
    GetUserTime(&end_time);
    copy_time = TimeDifference(&start_time, &end_time);

    GetUserTime(&start_time);
    for (n = 0; n < count; ++n) {
      int m;
      
      memcpy(z, true, sizeof(*z) * fft_size);
      av_rdft_calc(fft_inv_spec, (FFTSample*) z);

      for (m = 0; m < fft_size; ++m) {
        z[m] *= scale;
      }
    }
    GetUserTime(&end_time);

    elapsed_time = TimeDifference(&start_time, &end_time);

    if (verbose > 255) {
      printf("IFFT time          :  %g sec\n", elapsed_time);
      printf("Time for copying   :  %g sec\n", copy_time);
    }

    elapsed_time -= copy_time;

    if (verbose > 255)
      printf("Effective IFFT time:  %g sec\n", elapsed_time);

    PrintResultNoSNR("Inverse FFmpeg RFFT", fft_log_size, elapsed_time, count);
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
  free(fft_fwd_spec);
  free(fft_inv_spec);
}

void TimeFFmpegRFFT(int count, float signal_value, int signal_type) {
  int k;
  int min_order;

  if (verbose == 0)
    printf("%s FFmpeg RFFT\n", do_forward_test ? "Forward" : "Inverse");
  
  /* The minimum FFT order for rdft is 4. */

  min_order = min_fft_order < 4 ? 4 : min_fft_order;
  
  for (k = min_order; k <= max_fft_order; ++k) {
    int testCount = ComputeCount(count, k);
    TimeOneFFmpegRFFT(testCount, k, signal_value, signal_type);
  }
}

#endif
