# openmax-hacks

Clone of webrtc OpenMAX DL

This is a clone of WebRTC's OpenMAX DL for personal hacking.  No
guarantees that this is up-to-date or even compatible with WebRTC's
version.

## How to Use

While the code can be used independently, this repo assumes that it is
checked out inside a Chromium tree. In particular it should be cloned
in the third_party directory.

Thus

```
cd $CHROMIUM/src/third_party
git clone https://github.com/rtoy/openmax-hacks.git
cd openmax-hacks
```

### Building

Use
```
build/gyp_chromium --depth=$PWD OS=android third_party/openmax-hacks/dl/sp/src/test/test_fft_time.gyp
ninja -C out/Release
```

This creates timing tests for the OpenMAX DL routines.  You can add
the following flags to the gyp_chromium command line to enable tests
with other FFTs:

 * `-Dkissfft` enables [Kiss FFT](http://sourceforge.net/projects/kissfft/)
 * `-Dne10` enables [Ne10](https://github.com/projectNe10/Ne10)
 * `-Dffmpeg` enables [FFMPEG](https://www.ffmpeg.org/)
 * `-Dckfft` enables [Cricket FFT](http://www.crickettechnology.com/ckfft)
 * `-Dpffft` enables [PFFFT](https://bitbucket.org/jpommier/pffft/overview)

(Not all platforms support all of these options, but these should all
work on an arm7 with NEON.)

Other useful options:

 * `target_arch=arm64` to create an arm64 build. (Default is arm.)
 * `-Dne10_arm64_inline_asm` enables inline assembly for Ne10 arm64 builds.

Instead of `test_fft_time.gyp`, you can also use `test_fft.gyp`.  The
difference is that `test_fft_time.gyp` builds `test_fft_time` to do
timing tests for various FFTs.  `test_fft.gyp` builds a set of test
programs for testing the FFTs available in OpenMAX DL.

### Running

Once `test_fft_time` has been built, you can run it on an Android
device as follows:

```
adb root
adb push out/Release/test_fft_time <directory>
adb shell <directory>/test_fft_time -h
```

where `<directory>` is some suitable directory.  (On production
devices you cannot get root access, so this is not going to work for
you.)

The `-h` option prints out a help message.  Choose the appropriate set
of options for the test that you want to run.  In particular `-v0`
produces a brief output that is suitable for pasting into a
spreadsheet.  Large values for `-v` will produce increasingly verbose
output that might be useful for debugging.

