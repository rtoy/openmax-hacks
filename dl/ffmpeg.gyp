{
  'targets': [
    {
      'target_name': 'ffmpeg',
      'type': 'static_library',
      'include_dirs': [
        'ffmpeg/',
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
        'HAVE_FFMPEG',
      ],
      'sources': [
        'ffmpeg/config.h',

        'ffmpeg/libavcodec/arm/cpu.c',
        'ffmpeg/libavcodec/arm/fft_init_arm.c',
        'ffmpeg/libavcodec/arm/fft_neon.S',
        'ffmpeg/libavcodec/arm/mdct_neon.S',
        'ffmpeg/libavcodec/avfft.c',
        'ffmpeg/libavcodec/avfft.h',
        'ffmpeg/libavcodec/cpu.c',
        'ffmpeg/libavcodec/dct.h',
        'ffmpeg/libavcodec/fft.c',
        'ffmpeg/libavcodec/fft.h',
        'ffmpeg/libavcodec/fft-internal.h',
        'ffmpeg/libavcodec/mdct.c',
        'ffmpeg/libavcodec/rdft.h',
        'ffmpeg/libavcodec/synth_filter.h',

        'ffmpeg/libavutil/arm/asm.S',
        'ffmpeg/libavutil/arm/cpu.c',
        'ffmpeg/libavutil/arm/cpu.h',
        'ffmpeg/libavutil/attributes.h',
        'ffmpeg/libavutil/avconfig.h',
        'ffmpeg/libavutil/avutil.h',
        'ffmpeg/libavutil/bswap.h',
        'ffmpeg/libavutil/common.h',
        'ffmpeg/libavutil/cpu.c',
        'ffmpeg/libavutil/cpu.h',
        'ffmpeg/libavutil/dict.h',
        'ffmpeg/libavutil/error.h',
        'ffmpeg/libavutil/intfloat.h',
        'ffmpeg/libavutil/intfloat_readwrite.h',
        'ffmpeg/libavutil/intreadwrite.h',
        'ffmpeg/libavutil/log.h',
        'ffmpeg/libavutil/mathematics.h',
        'ffmpeg/libavutil/mem.c',
        'ffmpeg/libavutil/mem.h',
        'ffmpeg/libavutil/old_pix_fmts.h',
        'ffmpeg/libavutil/opt.h',
        'ffmpeg/libavutil/pixfmt.h',
        'ffmpeg/libavutil/rational.h',
        'ffmpeg/libavutil/samplefmt.h',
        'ffmpeg/libavutil/version.h',
      ],
  }],
}
