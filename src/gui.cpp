// gui.cpp
//
// Tcl/Tk-based GUI controller implementation.
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp

#include "gui.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

GUI::GUI(int argc, char** argv) {
    const char* command = "wish TkTempoConv3.tcl";
    if (!(fpipe = (FILE*)popen(command, "r"))) {
        perror("Problems with pipe");
        exit(1);
    }
    std::cerr << "created GUI." << std::endl;
}

GUI::~GUI() {
    pclose(fpipe);
}

void GUI::start() {
    GUI_thread_ = std::thread([this]() { getGUIparameters_(); });
}

void GUI::getGUIparameters_() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        char line[1024];
        char label[32];
        int  value = 0;
        if (fgets(line, sizeof(line), fpipe) != nullptr) {
            sscanf(line, "%31s %d\n", label, &value);
            if      (strcmp(label, "key")    == 0) { key.set(value); }
            else if (strcmp(label, "tempo")  == 0) { tempo.set(value); }
            else if (strcmp(label, "volume") == 0) { volume.set(value); }
            else if (strcmp(label, "stop")   == 0) { setstop(); }
            else if (strcmp(label, "quit")   == 0) { setquit(); }
            std::cerr << label << " " << value << std::endl;
        }
    }
}
