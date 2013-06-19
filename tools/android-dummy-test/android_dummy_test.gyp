# Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'targets': [
    {
      'target_name': 'android_dummy_test',
      'type': 'none',
      'copies': [
        {
          'destination': '<(PRODUCT_DIR)/simple_apk',
          'files': [
            'simple_apk/simple-debug.apk',
            'simple_apk/simple-unsigned.apk',
          ],
        },
      ],
    },
  ],
}
