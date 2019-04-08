#ifndef FFT_TIME_NE10_H_
#define FFT_TIME_NE10_H_

#if defined(HAVE_NE10)
void TimeOneNE10FFT(int count, int fft_log_size, float signal_value,
		    int signal_type);
void TimeNE10FFT(int count, float signal_value, int signal_type);
void TimeOneNE10RFFT(int count, int fft_log_size, float signal_value,
		    int signal_type);
void TimeNE10RFFT(int count, float signal_value, int signal_type);
#endif

#endif
