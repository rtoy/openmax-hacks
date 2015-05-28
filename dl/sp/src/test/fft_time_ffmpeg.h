#ifndef FFT_TIME_FFMPEG_H_
#define FFT_TIME_FFMPEG_H_

#if defined(HAVE_FFMPEG)
void TimeOneFFmpegFFT(int count, int fft_log_size, float signal_value,
		      int signal_type);
void TimeFFmpegFFT(int count, float signal_value, int signal_type);
void TimeOneFFmpegRFFT(int count, int fft_log_size, float signal_value,
                       int signal_type);
void TimeFFmpegRFFT(int count, float signal_value, int signal_type);
#endif

#endif
