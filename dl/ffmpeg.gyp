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
        'ffmpeg/libavcodec/avfft.h',
        'ffmpeg/libavcodec/avfft.c',
        'ffmpeg/libavcodec/dct.h',
        'ffmpeg/libavcodec/fft.h',
        'ffmpeg/libavcodec/rdft.h',
        'ffmpeg/libavcodec/attributes.h',
        'ffmpeg/libavcodec/avconfig.h',
        'ffmpeg/libavcodec/avutil.h',
        'ffmpeg/libavcodec/common.h',
        'ffmpeg/libavcodec/error.h',
        'ffmpeg/libavcodec/intfloat.h',
        'ffmpeg/libavcodec/intfloat_readwrite.h',
        'ffmpeg/libavcodec/log.h',
        'ffmpeg/libavcodec/mathematics.h',
        'ffmpeg/libavcodec/mem.h',
        'ffmpeg/libavcodec/old_pix_fmts.h',
        'ffmpeg/libavcodec/pixfmt.h',
        'ffmpeg/libavcodec/rational.h',
        'ffmpeg/libavcodec/version.h',
      ],
  }],
}
