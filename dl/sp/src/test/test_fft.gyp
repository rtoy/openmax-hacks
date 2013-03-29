#  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
#
#  Use of this source code is governed by a BSD-style license
#  that can be found in the LICENSE file in the root of the source
#  tree. An additional intellectual property rights grant can be found
#  in the file PATENTS.  All contributing project authors may
#  be found in the AUTHORS file in the root of the source tree.

{
  'variables' : {
    # Override this value to build with small float FFT tables
    'big_float_fft%' : 1,
    # Include KissFFT.  Default is no.
    'kissfft%' : 0,
    # Include NE10 FFTs.  Default is no.
    'ne10%' : 0,
    # Include FFMPEG FFTs.  Default is no.
    'ffmpeg%' : 0,
  },
  'target_defaults': {
    'include_dirs': [
      '../../../../',
    ],
    'dependencies' : [
      '../../../dl.gyp:openmax_dl',
      'test_utilities'
    ],
    'conditions': [
      ['big_float_fft == 1', {
        'defines': [
          'BIG_FFT_TABLE',
        ],
      }],
      ['kissfft == 1', {
        'defines': [
          'HAVE_KISSFFT',
        ],
        'dependencies' : [
          '../../../kissfft.gyp:kissfft',
        ]
      }],
      ['ne10 == 1', {
        'defines': [
          'HAVE_NE10',
        ],
        'dependencies' : [
          '../../../ne10.gyp:ne10',
        ]
      }],
      ['ffmpeg == 1', {
        'defines': [
          'HAVE_FFMPEG',
        ],
        'dependencies' : [
          '../../../ffmpeg.gyp:ffmpeg',
        ]
      }],
    ],
  },
  'targets': [
    {
      # Test utilities
      'target_name': 'test_utilities',
      'type' : '<(component)',
      'dependencies!' : [
        'test_utilities'
      ],
      'sources' : [
        'aligned_ptr.c',
        'compare.c',
        'gensig.c',
        'test_util.c',
      ],
    },
    {
      # Test complex fixed-point 16-bit FFT
      'target_name': 'test_fft16',
      'type': 'executable',
      'sources': [
        'test_fft16.c',
      ],
    },
    {
      # Test complex fixed-point 32-bit FFT
      'target_name': 'test_fft32',
      'type': 'executable',
      'sources': [
        'test_fft32.c',
      ],
    },
    {
      # Test real 32-bit fixed-point FFT
      'target_name': 'test_rfft32',
      'type': 'executable',
      'sources': [
        'test_rfft32.c',
      ],
    },
    {
      # Test real 16-bit fixed-point FFT
      'target_name': 'test_rfft16',
      'type': 'executable',
      'sources': [
        'test_rfft16.c',
      ],
    },
    {
      # Test complex floating-point FFT
      'target_name': 'test_float_fft',
      'type': 'executable',
      'sources': [
        'test_float_fft.c',
      ],
    },
    {
      # Test real floating-point FFT
      'target_name': 'test_float_rfft',
      'type': 'executable',
      'sources': [
        'test_float_rfft.c',
      ],
    },
    {
      # Simple timing test of FFTs
      'target_name': 'test_fft_time',
      'type': 'executable',
      'sources': [
        'test_fft_time.c',
      ],
    },
    {
      # Build all test programs.
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
        'test_fft16',
        'test_fft32',
        'test_float_fft',
        'test_float_rfft',
        'test_rfft16',
        'test_rfft32',
        'test_fft_time',
      ],
    },
  ],
}
