{
  'targets': [
    {
      'target_name': 'ckfft',
      'type': 'static_library',
      'include_dirs': [
        'ckfft-1.0/inc/',
        'ckfft-1.0/src/',
      ],
      'cflags!': [
        '-mfpu=vfpv3-d16',
      ],
      'cflags': [
        # We enable Neon instructions even with arm_neon==0, to support
        # runtime detection.
        '-mfpu=neon',
      ],
      'defines': [
        'CKFFT_ARM_NEON',
      ],
      'dependencies': [
        '<(android_ndk_root)/android_tools_ndk.gyp:cpu_features',
      ],
      'sources': [
        'ckfft-1.0/inc/ckfft/ckfft.h',
        'ckfft-1.0/src/ckfft/ckfft.cpp',
        'ckfft-1.0/src/ckfft/context.cpp',
        'ckfft-1.0/src/ckfft/context.h',
        'ckfft-1.0/src/ckfft/debug.cpp',
        'ckfft-1.0/src/ckfft/debug.h',
        'ckfft-1.0/src/ckfft/fft.cpp',
        'ckfft-1.0/src/ckfft/fft.h',
        'ckfft-1.0/src/ckfft/fft_default.cpp',
        'ckfft-1.0/src/ckfft/fft_default.h',
        'ckfft-1.0/src/ckfft/fft_neon.cpp',
        'ckfft-1.0/src/ckfft/fft_neon.h',
        'ckfft-1.0/src/ckfft/fft_real.cpp',
        'ckfft-1.0/src/ckfft/fft_real.h',
        'ckfft-1.0/src/ckfft/fft_real_default.cpp',
        'ckfft-1.0/src/ckfft/fft_real_default.h',
        'ckfft-1.0/src/ckfft/fft_real_neon.cpp',
        'ckfft-1.0/src/ckfft/fft_real_neon.h',
        'ckfft-1.0/src/ckfft/math_util.h',
        'ckfft-1.0/src/ckfft/platform.h',
      ],
  }],
}
