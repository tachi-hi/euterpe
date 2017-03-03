# EUTERPE

This is an automatic audio-to-audio karaoke generation system.
It converts an ordinary music signal into a karaoke (vocal-off) in real time.
it can also convert the key of the song.

For technical details, see the literatures below, especially the latest one [1].

## Related Articles

[1] Tachibana, H., Mizuno, Y., Ono, N. and Sagayama, S.: A Real-time Audio-to-audio Karaoke Generation System for Monaural Recordings Based on Singing Voice Suppression and Key Conversion Techniques, Journal of Information Processing, Vol. 24, No. 3, pp. 470–482, May, 2016.

+ [Article @ IPSJ Digital Library](http://id.nii.ac.jp/1001/00160331/)
+ [Supplementary Material (Video Demo)](http://id.nii.ac.jp/1012/00000006/)

[2] My (H. Tachibana's) Ph.D. Thesis, The University of Tokyo, Mar., 2014.

[3] Tachibana, H., Mizuno, Y., Ono, N. and Sagayama, S.: Euterpe: A Real-time Automatic Karaoke Generation System based on Singing Voice Suppression and Pitch Conversion, Proc. ASJ autumn meeting, Sep., 2012, **Non-refereed,** **in Japanese**

## Setting Up

### Hardware and OS

+ Hardware: Audio I/O is required. (PC with microphones and speakers)
+ OS: Linux and Mac OS X
    + The system has been originally developed on Linux Mint.
    + It would be also easy to build the system on other Debian-based distributions including Ubuntu.
    + It may also be possible to build the system on other kind of distributions e.g. Fedora, but I have not verified it yet.
    + I have successfully built the codes on Mac OS X (Yosemite) using `clang++`

### Libraries

You need to install following softwares first.

    g++
    tcl/tk
    PortAudio (libportaudio-dev)
    fftw3 (libfftw3-dev)

### Build

Just run the `makefile` in `src`

    cd src
    make
    cd -

### Usage

- Buy a 3.5mm-3.5mm audio cable.
- Make your audio player (such as iPod) ready.
- Connect one end of the cable to your audio player's output jack (3.5mm earphone jack), and the oppisite end to your PC's audio input jack.
- Run the system by the command below.

    ./euterpe


## See Also

- Other codes used in the papers above
   - <https://github.com/tachi-hi/temperament4fft>
   - <https://github.com/tachi-hi/HPSS> (In the emperical evaluations in the paper above, the codes in this repository were used.)
- Related codes
    - <https://github.com/tachi-hi/slidingHPSS> (An older implementation of HPSS)
