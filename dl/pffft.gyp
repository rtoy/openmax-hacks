{
  'targets': [
    {
      'target_name': 'pffft',
      'type': 'static_library',
      'include_dirs': [
        'pffft',
      ],
      'cflags!': [
        '-mfpu=vfpv3-d16',
      ],
      'cflags': [
        # We enable Neon instructions even with arm_neon==0, to support
        # runtime detection.
        '-mfpu=neon',
      ],
      'sources' : [
        'pffft/fftpack.c',
        'pffft/fftpack.h',
        'pffft/pffft.c',
        'pffft/pffft.h',
      ],
  },
  {
      'target_name': 'test_pffft',
      'type': 'executable',
      'include_dirs': [
        'pffft',
      ],
      'dependencies': [
        'pffft',
      ],
      'cflags!': [
        '-mfpu=vfpv3-d16',
      ],
      'cflags': [
        # We enable Neon instructions even with arm_neon==0, to support
        # runtime detection.
        '-mfpu=neon',
      ],
      'sources' : [
        'pffft/test_pffft.c',
      ],
  }],
}
