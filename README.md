# openmax-hacks

Clone of webrtc OpenMAX DL

This is a clone of WebRTC's OpenMAX DL for personal hacking.  No guarantees that this is up-to-date or even compatible with WebRTC's version.

## How to Use

While the code can be used independently, this repo assumes that it
is checked out inside a Chromium tree. In particular it should be cloned in the
third_party directory.

To build, use
```
build/gyp_chromium --depth=$PWD third_party/openmax-hacks/dl/sp/src/test/test_fft_time.gyp
ninja -C out/Release
```

Instead of `test_fft_time.gyp`, you can also use `test_fft.gyp`.  The difference is that `test_fft_time.gyp` builds `test_fft_time` to do timing tests for various FFTs.  `test_fft.gyp` builds a set of test programs for testing the FFTs available in OpenMAX DL.

Look through the gyp files to see what options are available.
