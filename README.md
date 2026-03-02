# EUTERPE

This is an automatic audio-to-audio karaoke generation system.
It converts an ordinary music signal into a karaoke (vocal-off) in real time.
it can also convert the key of the song.

For technical details, see the literatures below, especially the latest one (Tachibana 2016).

## Related Articles

### Journal (IPSJ JIP)
+ Paper: [[J-STAGE]](https://doi.org/10.2197/ipsjjip.24.470), [[IPSJ Digital Library]](http://id.nii.ac.jp/1001/00160331/)
    + [Supplementary Material (Video Demo)](http://id.nii.ac.jp/1012/00000006/)
```bibtex
@article{tachibana2016ipsjjip,
    author = {H. Tachibana and Y. Mizuno and N. Ono and S. Sagayama},
    title = {A Real-time Audio-to-audio Karaoke Generation System for Monaural Recordings 
            Based on Singing Voice Suppression and Key Conversion Techniques},
    journal = {Journal of Information Processing},
    publisher = {Information Processing Society of Japan},
    volume = 24,
    number = 3, 
    pages = 470–-482, 
    month = May,
    year = 2016,
    doi = {10.2197/ipsjjip.24.470}
}
```

### My PhD dissertation

```bibtex
@phdthesis{tachibana2014thesis,
    author = {H. Tachibana},
    title = {Music signal processing exploiting spectral fluctuation of singing voice 
            using harmonic/percussive sound separation},
    school = {The University of Tokyo},
    month = Mar.,
    year = 2014
}
```

### Conference Proceedings

```bibtex
@inproceedings{tachibana2012asj,
    author = {H. Tachibana and Y. Mizuno and N. Ono and S. Sagayama},
    title = {Euterpe: A Real-time Automatic Karaoke Generation System 
            based on Singing Voice Suppression and Pitch Conversion}, 
    booktitle = "Proc. ASJ autumn meeting",
    month = Sep.,
    year = 2012,
    note = "Non-refereed, in Japanese"
}
```

## Setting Up

### Hardware and OS

+ Hardware: Audio I/O is required. (PC with microphones and speakers)
+ OS: Linux and Mac OS X
    + The system has been originally developed on Linux Mint.
    + It would be also easy to build the system on other Debian-based distributions including Ubuntu.
    + It may also be possible to build the system on other kind of distributions e.g. Fedora, but I have not verified it yet.
    + I have successfully built the codes on Mac OS X (Yosemite) using `clang++`

### Prerequisites

+ CMake (>= 3.15)
+ Git
+ Tcl/Tk (`wish` command)
    + On macOS: `brew install tcl-tk`
    + On Debian/Ubuntu: `sudo apt-get install tcl tk`

Libraries (PortAudio and FFTW3) are automatically installed via [vcpkg](https://github.com/microsoft/vcpkg).

### Build

Run `build.sh` at the project root. It handles vcpkg setup, CMake configuration, and build automatically.

    ./build.sh          # Release build (default)
    ./build.sh debug    # Debug build
    ./build.sh clean    # Remove build artifacts

The executable will be created at `build/euterpe`.

<details>
<summary>Alternative: Manual build with system libraries</summary>

If you prefer using system-installed libraries instead of vcpkg:

    sudo apt-get install g++ libportaudio-dev libfftw3-dev  # Debian/Ubuntu

    cd src
    make
    cd -

</details>

### Usage

- Buy a 3.5mm-3.5mm audio cable.
- Make your audio player (such as iPod) ready.
- Connect one end of the cable to your audio player's output jack (3.5mm earphone jack), and the opposite end to your PC's audio input jack.
- Run the system by the command below.

      cd build
      ./euterpe


## See Also

- Other codes used in the papers above
   - <https://github.com/tachi-hi/temperament4fft>
   - <https://github.com/tachi-hi/HPSS> (In the emperical evaluations in the paper above, the codes in this repository were used.)
- Related codes
    - <https://github.com/tachi-hi/slidingHPSS> (An older implementation of HPSS)

## On the name "Euterpe"

The name "Euterpe" is based on the Sagayama Lab's practice of adopting project names from Greek myth.
If you enjoyed Euterpe, please check out the lab's other projects.

- [Eurydice (Automatic Accompaniment System)](http://hil.t.u-tokyo.ac.jp/software/Eurydice/index-e.html)
- [Orpheus (Automatic Composition from Japanese Lyrics)](http://www.orpheus-music.org/index.php)
    - Melete (an Orpheus clone)
    
