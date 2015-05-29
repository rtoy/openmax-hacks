# See http://www.crickettechnology.com/ckfft
{
  'targets': [
    {
      'target_name': 'ckfft',
      'type': 'static_library',
      'include_dirs': [
        'ckfft/inc/',
        'ckfft/src/',
      ],
      'conditions' : [
        ['target_arch == "arm"', {
          'cflags!': [
            '-mfpu=vfpv3-d16',
          ],
          'cflags': [
            # We enable Neon instructions even with arm_neon==0, to support
            # runtime detection.
            '-mfpu=neon',
          ],
        }],
      ],
      'defines': [
        'CKFFT_ARM_NEON',
      ],
      'includes': [
        '../../../../build/android/cpufeatures.gypi',
      ],
      'sources': [
        'ckfft/inc/ckfft/ckfft.h',
        'ckfft/src/ckfft/ckfft.cpp',
        'ckfft/src/ckfft/context.cpp',
        'ckfft/src/ckfft/context.h',
        'ckfft/src/ckfft/debug.cpp',
        'ckfft/src/ckfft/debug.h',
        'ckfft/src/ckfft/fft.cpp',
        'ckfft/src/ckfft/fft.h',
        'ckfft/src/ckfft/fft_default.cpp',
        'ckfft/src/ckfft/fft_default.h',
        'ckfft/src/ckfft/fft_neon.cpp',
        'ckfft/src/ckfft/fft_neon.h',
        'ckfft/src/ckfft/fft_real.cpp',
        'ckfft/src/ckfft/fft_real.h',
        'ckfft/src/ckfft/fft_real_default.cpp',
        'ckfft/src/ckfft/fft_real_default.h',
        'ckfft/src/ckfft/fft_real_neon.cpp',
        'ckfft/src/ckfft/fft_real_neon.h',
        'ckfft/src/ckfft/math_util.h',
        'ckfft/src/ckfft/platform.h',
      ],
  }],
}
