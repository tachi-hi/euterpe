# EUTERPE

Karaoke system.

## Related Articles

+ Tachibana, H., Mizuno, Y., Ono, N. and Sagayama, S.: A Real-time Audio-to-audio Karaoke Generation System for Monaural Recordings Based on Singing Voice Suppression and Key Conversion Techniques (submitting)
+ My (H. Tachibana's) Ph.D. Thesis
+ Tachibana, H., Mizuno, Y., Ono, N. and Sagayama, S.: Euterpe: A Real-time Automatic Karaoke Generation System based on Singing Voice Suppression and Pitch Conversion, Proc. ASJ autumn meeting (2012), **Non-refereed** **in Japanese**

## Setting Up

### Hardware and OS

+ Hardware: Audio I/O is required. (PC with microphones and speakers)
+ OS: The system has been developed on the Linux Mint. 
    + It would be easier to build the systme on other Debian-based distributions. 
    + It may also be possible to build the system on other kind of distributions e.g. Fedora but I have not verified yet.

### Libraries

You need to install following softwares first.

    g++
    tcl/tk
    PortAudio (libportaudio-dev)
    fftw3 (libfftw3-dev)

### Build

Just run the `makefile`

    make

### How to run?

    ./euterpe


## See Also

- <https://github.com/tachi-hi/temperament4fft>
- <https://github.com/tachi-hi/slidingHPSS>

