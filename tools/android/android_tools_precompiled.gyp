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
      # Copies selected precompiled binaries of the md5sum and forwarder2
      # directories in Chromium's tools/android. They are needed to be able to
      # run Android tests in WebRTC.
      'target_name': 'android_tools_precompiled',
      'type': 'none',
      'copies': [
        {
          # Binary run on the device, so it's the same for Mac and Linux.
          'destination': '<(PRODUCT_DIR)/md5sum_dist',
          'files': [
            'target-<(target_arch)/md5sum_dist/md5sum_bin',
          ],
        },
        {
          'destination': '<(PRODUCT_DIR)',
          'files': [
            '<(host_os)/host_forwarder',
            '<(host_os)/md5sum_bin_host',
          ],
        },
      ],
    },
  ],
}
