#ifndef FFT_TIME_KISSFFT_H_
#define FFT_TIME_KISSFFT_H_
#if defined(HAVE_KISSFFT)

void TimeOneKissFFT(int count, int fft_log_size, float signal_value,
		    int signal_type);
void TimeKissFFT(int count, float signal_value, int signal_type);

void TimeOneKissRFFT(int count, int fft_log_size, float signal_value,
                     int signal_type);
void TimeKissRFFT(int count, float signal_value, int signal_type);
#endif
#endif
