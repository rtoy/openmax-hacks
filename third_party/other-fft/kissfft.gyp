# See http://sourceforge.net/projects/kissfft/
{
  'targets': [
    {
      'target_name': 'kissfft',
      'type': 'static_library',
      'include_dirs': [
        '../',
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
        '__ARM_HAVE_NEON',
        'HAVE_KISSFFT',
      ],
      'sources': [
        'kiss_fft130/_kiss_fft_guts.h',
        'kiss_fft130/kiss_fft.c',
        'kiss_fft130/kiss_fft.h',
        'kiss_fft130/kiss_fft_bfly2_neon.S',
        'kiss_fft130/kiss_fft_bfly4_neon.S',
      ],
  }],
}
