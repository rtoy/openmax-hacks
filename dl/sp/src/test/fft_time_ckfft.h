#ifndef FFT_TIME_CKFFT_H_
#define FFT_TIME_CKFFT_H_
#if defined(HAVE_CKFFT)
void TimeOneCkFFTFFT(int count, int fft_log_size, float signal_value,
		      int signal_type);
void TimeCkFFTFFT(int count, float signal_value, int signal_type);
void TimeOneCkFFTRFFT(int count, int fft_log_size, float signal_value,
                       int signal_type);
void TimeCkFFTRFFT(int count, float signal_value, int signal_type);
#endif

#endif
