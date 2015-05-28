#ifndef FFT_TIME_PFFFT_H_
#define FFT_TIME_PFFFT_H_
#if defined(HAVE_PFFFT)
void TimeOnePfFFT(int count, int fft_log_size, float signal_value,
                  int signal_type);
void TimePfFFT(int count, float signal_value, int signal_type);

void TimeOnePfRFFT(int count, int fft_log_size, float signal_value,
                   int signal_type);
void TimePfRFFT(int count, float signal_value, int signal_type);

#endif

#endif
