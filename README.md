# EUTERPE

This is an automatic audio-to-audio karaoke generation system.
It converts an ordinary music signal into a karaoke (vocal-off) in real time.
It can also convert the key of the song.

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
+ OS: Linux and macOS
    + Developed and tested on Linux (Ubuntu/Debian) and macOS.

### Prerequisites

+ CMake (>= 3.15)
+ Git
+ C++17-compatible compiler (GCC 9+ or Clang 10+)

Libraries (PortAudio, FFTW3, Dear ImGui) are automatically installed via [vcpkg](https://github.com/microsoft/vcpkg).

### Build

Run `build.sh` at the project root. It handles vcpkg setup, CMake configuration, and build automatically.

    ./build.sh          # Release build — double precision (default)
    ./build.sh float    # Release build — float precision (faster on some hardware)
    ./build.sh debug    # Debug build — double precision
    ./build.sh float-debug  # Debug build — float precision
    ./build.sh clean    # Remove build artifacts

The executable will be created at `build/euterpe`.

### Usage

- Buy a 3.5mm-3.5mm audio cable.
- Make your audio player (such as a smartphone) ready.
- Connect one end of the cable to your audio player's output jack (3.5mm earphone jack), and the opposite end to your PC's audio input jack.
- Run the system:

      cd build
      ./euterpe           # Standard mode (max ~3 iterations/frame)
      ./euterpe -m        # High-quality mode (unlimited iterations, higher CPU)

A Dear ImGui-based GUI will appear, providing real-time control of:
- **Key** — pitch shift (-12 to +12 semitones)
- **Volume** — output volume
- **Stop / Quit** — playback control

### Quality Mode

By default, the HPSS and phase recovery loops are limited to approximately 3 iterations per frame to keep CPU usage moderate.
Use the `-m` / `--max-iter` flag to remove this limit, allowing the system to iterate as many times as possible before the next audio frame arrives (higher quality, higher CPU load).

## See Also

- Other codes used in the papers above
   - <https://github.com/tachi-hi/temperament4fft>
   - <https://github.com/tachi-hi/HPSS> (In the empirical evaluations in the paper above, the codes in this repository were used.)
- Related codes
    - <https://github.com/tachi-hi/slidingHPSS> (An older implementation of HPSS)

## On the name "Euterpe"

The name "Euterpe" is based on the Sagayama Lab's practice of adopting project names from Greek myth.
If you enjoyed Euterpe, please check out the lab's other projects.

- [Eurydice (Automatic Accompaniment System)](http://hil.t.u-tokyo.ac.jp/software/Eurydice/index-e.html)
- [Orpheus (Automatic Composition from Japanese Lyrics)](http://www.orpheus-music.org/index.php)
    - Melete (an Orpheus clone)
