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

void x86SP_FFT_CToC_FC32_Inv_Radix2_fs(
    OMX_F32 *in,
    OMX_F32 *out,
    OMX_INT n) {
  OMX_INT i;
  OMX_INT n_by_2 = n >> 1;
  OMX_F32 *out0 = out;

  for (i = 0; i < n_by_2; i++) {
    OMX_F32 *in0 = in + i;
    OMX_F32 *in1 = in0 + n_by_2;
    OMX_F32 *out1 = out0 + n_by_2;

    // CADD out0, in0, in1
    out0[0] = in0[0] + in1[0];
    out0[n] = in0[n] + in1[n];

    // CSUB out1, in0, in1
    out1[0] = in0[0] - in1[0];
    out1[n] = in0[n] - in1[n];

    out0 += 1;
  }
}
