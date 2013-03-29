{
  'targets': [
    {
      'target_name': 'ne10',
      'type': 'static_library',
      'include_dirs': [
        '../',
        'Ne10/inc',
        'Ne10/common',
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
        'HAVE_NE10',
      ],
      'sources' : [
        'Ne10/common/NE10_mask_table.c',
        'Ne10/common/NE10_mask_table.h'
        'Ne10/inc/NE10_types.h',
        'Ne10/modules/dsp/NE10_cfft.c',
        'Ne10/modules/dsp/NE10_cfft_init.c',
        'Ne10/modules/dsp/NE10_cfft.neon.s',
        'Ne10/modules/dsp/NE10_fir.c',
        'Ne10/modules/dsp/NE10_fir_init.c',
        'Ne10/modules/dsp/NE10_fir.neon.s',
        'Ne10/modules/dsp/NE10_iir.c',
        'Ne10/modules/dsp/NE10_iir_init.c',
        'Ne10/modules/dsp/NE10_iir.neon.s',
        'Ne10/modules/dsp/NE10_init_dsp.c',
        'Ne10/modules/dsp/NE10_rfft.c',
        'Ne10/modules/dsp/NE10_rfft_init.c',
        'Ne10/modules/dsp/NE10_rfft.neon.c',
      ],
  }],
}
