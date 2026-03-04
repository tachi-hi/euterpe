// euterpe_qt.cpp
//
// Qt6-based entry point for euterpe.
// Supports both line-in (PortAudio) and WAV file input.
// Uses GUIBase (AtomicParam) instead of the Tcl/Tk GUI class.
//
// Pipeline:
//   AudioSource (LineAudioSource or FileAudioSource)
//     -> inBuffer -> HPSS_long -> HPSS_short -> StreamAdder
//     -> TempoPitch -> outBuffer -> PortAudio output

#include <QApplication>
#include <QTimer>
#include <iostream>

#include "guiBase.h"
#include "streamBuffer.h"
#include "HPSS_pipe_long.h"
#include "streamAdder.h"
#include "tempoPitch.h"
#include "lineAudioSource.h"
#include "fileAudioSource.h"
#include "qtApp/mainWindow.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    constexpr int freq = 16000;
    constexpr int spb  = 1024;

    // --- Stream buffers ---
    StreamBuffer<float> inBuffer;
    StreamBuffer<float> Buffer_H_short;
    StreamBuffer<float> Buffer_P_short;
    StreamBuffer<float> Buffer_H_long;
    StreamBuffer<float> Buffer_P_long;
    StreamBuffer<float> Buffer_karaoke;
    StreamBuffer<float> outBuffer;

    // --- Shared parameter store ---
    GUIBase params;

    // --- Signal processing pipeline ---
    HPSS_pipe  HPSS_short(7,  512,  256, 10, 10);
    HPSS_short.setBuffer(&Buffer_P_long, &Buffer_H_short, &Buffer_P_short);

    HPSS_pipe_long HPSS_long(7, 2048, 1024, 10, 10);
    HPSS_long.setBuffer(&inBuffer, &Buffer_H_long, &Buffer_P_long);

    StreamAdder adder(2048, &params);
    adder.add_input_stream(&Buffer_P_short); adder.set_multiply(0, 1.0);
    adder.add_input_stream(&Buffer_H_long);  adder.set_multiply(1, 1.0);
    adder.add_input_stream(&Buffer_H_short); adder.set_multiply(2, 0.0);
    adder.set_output_stream(&Buffer_karaoke);

    TempoPitch converter;
    converter.init(1024 * 2, 128 * 2, 1, 7, 15, freq, &params);
    converter.setBuffer(&Buffer_karaoke, &outBuffer);

    // --- Qt main window ---
    MainWindow window(&params);
    window.show();

    // Audio source (created on Start)
    std::unique_ptr<AudioSource> source;

    // Connect Start button: initialize audio source and start pipeline
    QObject::connect(&window, &MainWindow::startRequested, [&]() {
        if (window.isFileMode()) {
            auto path = window.selectedFile();
            if (path.empty()) {
                std::cerr << "No file selected." << std::endl;
                return;
            }
            source = std::make_unique<FileAudioSource>(path, freq, spb, outBuffer);
        } else {
            source = std::make_unique<LineAudioSource>(freq, spb, outBuffer);
        }

        HPSS_short.start();
        HPSS_long.start();
        adder.start();
        converter.start();
        source->start(inBuffer);

        // Periodic status log (every 1 s)
        auto* timer = new QTimer(&app);
        QObject::connect(timer, &QTimer::timeout, [&]() {
            std::cerr
                << "buf: in="    << inBuffer.size()       / (freq / 1000) << "ms"
                << "  kar="      << Buffer_karaoke.size() / (freq / 1000) << "ms"
                << "  out="      << outBuffer.size()      / (freq / 1000) << "ms"
                << "  HPSS_s="   << HPSS_short.get_exec_count_diff() << "/s"
                << "  HPSS_l="   << HPSS_long.get_exec_count_diff()  << "/s"
                << "  pitch="    << converter.get_exec_count_diff()   << "/s"
                << std::endl;
        });
        timer->start(1000);
    });

    return app.exec();
}
