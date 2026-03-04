// gui.h
//
// Tcl/Tk-based GUI controller.
// Launches TkTempoConv3.tcl via popen and reads parameters line-by-line.
// Inherits shared parameter store from GUIBase.
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp

#pragma once
#include "guiBase.h"
#include <cstdio>
#include <thread>

class GUI : public GUIBase {
public:
    GUI(int argc, char** argv);
    ~GUI();

    void start();

private:
    FILE*       fpipe;
    std::thread GUI_thread_;

    void getGUIparameters_();
};
