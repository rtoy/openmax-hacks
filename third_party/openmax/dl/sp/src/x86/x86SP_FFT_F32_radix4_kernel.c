/*
 *  Copyright (c) 2013 Intel Corporation. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */
#include "dl/api/omxtypes.h"
#include <stdbool.h>

OMX_F32* x86SP_F32_radix4_kernel_OutOfPlace(
    OMX_F32 *src,
    OMX_F32 *buf1,
    OMX_F32 *buf2,
    OMX_F32 *twiddle,
    OMX_INT n,
    bool forward_fft) {
  OMX_INT sub_size;
  OMX_INT sub_num;
  OMX_INT n_by_4 = n >> 2;
  OMX_F32 *in = buf1;
  OMX_F32 *out = buf2;

  if (forward_fft)
    x86SP_FFT_CToC_FC32_Fwd_Radix4_fs(src, in, n);
  else
    x86SP_FFT_CToC_FC32_Inv_Radix4_fs(src, in, n);

  for (sub_size = 4, sub_num = n_by_4;
       sub_size < n_by_4;
       sub_size = sub_size << 2, sub_num = sub_num >> 2) {

    if (forward_fft) {
      x86SP_FFT_CToC_FC32_Fwd_Radix4_ms(in, out, twiddle,
                                        n, sub_size, sub_num);
    } else {
      x86SP_FFT_CToC_FC32_Inv_Radix4_ms(in, out, twiddle,
                                        n, sub_size, sub_num);
    }

    OMX_F32 *temp = out;
    out = in;
    in = temp;
  }

  if (forward_fft) {
    if (sub_num == 2)
      x86SP_FFT_CToC_FC32_Fwd_Radix2_ls(in, out, twiddle, n);
    else
      x86SP_FFT_CToC_FC32_Fwd_Radix4_ls(in, out, twiddle, n);
  } else {
    if (sub_num == 2)
      x86SP_FFT_CToC_FC32_Inv_Radix2_ls(in, out, twiddle, n);
    else
      x86SP_FFT_CToC_FC32_Inv_Radix4_ls(in, out, twiddle, n);
  }

  return out;
}

OMX_F32* x86SP_F32_radix4_kernel_OutOfPlace_sse(
    OMX_F32 *src,
    OMX_F32 *buf1,
    OMX_F32 *buf2,
    OMX_F32 *twiddle,
    OMX_INT n,
    // true for forward, false for inverse.
    bool forward_fft) {
  OMX_INT sub_size, sub_num;
  OMX_INT n_by_4 = n >> 2;
  OMX_F32 *in, *out;
  in = buf1;
  out = buf2;

  if (forward_fft)
    x86SP_FFT_CToC_FC32_Fwd_Radix4_fs_sse(src, in, n);
  else
    x86SP_FFT_CToC_FC32_Inv_Radix4_fs_sse(src, in, n);

  for (sub_size = 4, sub_num = n_by_4;
       sub_size < n_by_4;
       sub_size = sub_size << 2, sub_num = sub_num >> 2) {

    if (forward_fft) {
      x86SP_FFT_CToC_FC32_Fwd_Radix4_ms_sse(in, out, twiddle,
                                            n, sub_size, sub_num);
    } else {
      x86SP_FFT_CToC_FC32_Inv_Radix4_ms_sse(in, out, twiddle,
                                            n, sub_size, sub_num);
    }

    OMX_F32 *temp = out;
    out = in;
    in = temp;
  }

  // If n is not power of 4, sub_num == 2.
  if (forward_fft) {
    if (sub_num == 2)
      x86SP_FFT_CToC_FC32_Fwd_Radix2_ls_sse(in, out, twiddle, n);
    else
      x86SP_FFT_CToC_FC32_Fwd_Radix4_ls_sse(in, out, twiddle, n);
  } else {
    if (sub_num == 2)
      x86SP_FFT_CToC_FC32_Inv_Radix2_ls_sse(in, out, twiddle, n);
    else
      x86SP_FFT_CToC_FC32_Inv_Radix4_ls_sse(in, out, twiddle, n);
  }

  return out;
}
