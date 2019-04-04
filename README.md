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
mv openmax_dl openmax_dl-save
git clone https://github.com/rtoy/openmax-hacks.git openmax_dl
```

Moving `openmax_dl` directory out of the way isn't ideal, but for now,
this is the easiest way to get things going

### Building

Use
```
cd $CHROMIUM/src
# Fill in gn args
# I'm using
# target_os = "android"
# target_cpu = "arm"
# is_debug = false
gn args out/Release
ninja -C out/Release test_float_fft test_float_rfft test_fft_time
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
adb push out/Release/test_fft_time /data/local/tmp
adb shell /data/local/tmp/test_fft_time -h
```

where directory `/data/local/tmp` seems like a general available
suitable directory that can be used for installing the executable.  If
this doesn't work, find some other directory, or maybe try `adb root`
first.

The `-h` option prints out a help message.  Choose the appropriate set
of options for the test that you want to run.  In particular `-v0`
produces a brief output that is suitable for pasting into a
spreadsheet.  Large values for `-v` will produce increasingly verbose
output that might be useful for debugging.

